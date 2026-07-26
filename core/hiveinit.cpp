/*
 * PROJECT:   NTREG Kernel
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#define NDEBUG
#include "cmlib.hpp"

#include "debug.hpp"

/* ENUMERATIONS *************************************************************/

typedef enum _RESULT
{
    NotHive,        // 不是Hive文件
    Fail,           // 操作失败
    NoMemory,       // 内存不足
    HiveSuccess,    // Hive操作成功
    RecoverHeader,  // 恢复Hive头信息
    RecoverData,    // 恢复Hive数据
    SelfHeal        // 自我修复
} RESULT;

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief
 * 验证注册表文件（蜂巢或日志）的基础块头。
 *
 * @param[in] BaseBlock
 * 指向要验证的基础块头的指针。
 *
 * @param[in] FileType
 * 要检查的注册表文件类型，与基础块的文件类型进行比较。
 *
 * @return
 * 如果基础块头有效，则返回TRUE，否则返回FALSE。
 */
BOOLEAN
CMAPI
HvpVerifyHiveHeader(
    _In_ PHBASE_BLOCK BaseBlock,
    _In_ ULONG FileType)
{
    if (BaseBlock->Signature != HV_HBLOCK_SIGNATURE ||
        BaseBlock->Major != HSYS_MAJOR ||
        BaseBlock->Minor < HSYS_MINOR ||
        BaseBlock->Type != FileType ||
        BaseBlock->Format != HBASE_FORMAT_MEMORY ||
        BaseBlock->Cluster != 1 ||
        BaseBlock->Sequence1 != BaseBlock->Sequence2 ||
        HvpHiveHeaderChecksum(BaseBlock) != BaseBlock->CheckSum)
    {
        DPRINT1("Verify Hive Header failed:\n");
        DPRINT1("    Signature: 0x%x, expected 0x%x; Major: 0x%x, expected 0x%x\n",
                BaseBlock->Signature, HV_HBLOCK_SIGNATURE, BaseBlock->Major, HSYS_MAJOR);
        DPRINT1("    Minor: 0x%x expected to be >= 0x%x; Type: 0x%x, expected 0x%x\n",
                BaseBlock->Minor, HSYS_MINOR, BaseBlock->Type, FileType);
        DPRINT1("    Format: 0x%x, expected 0x%x; Cluster: 0x%x, expected 1\n",
                BaseBlock->Format, HBASE_FORMAT_MEMORY, BaseBlock->Cluster);
        DPRINT1("    Sequence: 0x%x, expected 0x%x; Checksum: 0x%x, expected 0x%x\n",
                BaseBlock->Sequence1, BaseBlock->Sequence2,
                HvpHiveHeaderChecksum(BaseBlock), BaseBlock->CheckSum);

        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * 释放与蜂巢描述符关联的存储空间中的所有存储桶。
 *
 * @param[in] Hive
 * 指向蜂巢描述符的指针，其中的所有存储桶将被释放。
 */
void
CMAPI
HvpFreeHiveBins(
    _In_ PHHIVE Hive)
{
    ULONG i;
    PHBIN Bin;
    ULONG Storage;

    for (Storage = 0; Storage < Hive->StorageTypeCount; Storage++)
    {
        Bin = NULL;
        for (i = 0; i < Hive->Storage[Storage].Length; i++)
        {
            if (Hive->Storage[Storage].BlockList[i].BinAddress == 0)
                continue;
            if (Hive->Storage[Storage].BlockList[i].BinAddress != reinterpret_cast<ULONG_PTR>(Bin))
            {
                Bin = reinterpret_cast<PHBIN>(Hive->Storage[Storage].BlockList[i].BinAddress);
                Hive->Free(reinterpret_cast<PHBIN>(Hive->Storage[Storage].BlockList[i].BinAddress), 0);
            }
            Hive->Storage[Storage].BlockList[i].BinAddress = 0;
            Hive->Storage[Storage].BlockList[i].BlockAddress = 0;
        }

        if (Hive->Storage[Storage].Length)
            Hive->Free(Hive->Storage[Storage].BlockList, 0);
    }
}

/**
 * @brief
 * 分配一个集群对齐的蜂巢基础头块。
 *
 * @param[in] Hive
 * 指向蜂巢描述符的指针，从中获取头块分配器函数。
 *
 * @param[in] Paged
 * 如果设置为TRUE，分配的基础块将位于分页池中，否则位于非分页池中。
 *
 * @param[in] Tag
 * 用于分配内存块的标签名称，用于识别。这是用于调试目的。
 *
 * @return
 * 如果函数成功，则返回分配的基础块头，否则返回NULL。
 */
static
__inline
PHBASE_BLOCK
HvpAllocBaseBlockAligned(
    _In_ PHHIVE Hive,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Alignment;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* 分配缓冲区 */
    BaseBlock = reinterpret_cast<PHBASE_BLOCK>(Hive->Allocate(Hive->BaseBlockAlloc, Paged, Tag));
    if (!BaseBlock) return NULL;

    /* 检查并强制对齐 */
    Alignment = Hive->Cluster * HSECTOR_SIZE -1;
    if (reinterpret_cast<ULONG_PTR>(BaseBlock) & Alignment)
    {
        /* 释放旧头并重新分配一个新的，总是分页的 */
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        BaseBlock = reinterpret_cast<PHBASE_BLOCK>(Hive->Allocate(sizeof(HBASE_BLOCK), TRUE, Tag));
        if (!BaseBlock) return NULL;

        Hive->BaseBlockAlloc = sizeof(HBASE_BLOCK);
    }

    return BaseBlock;
}

/**
 * @brief
 * 初始化蜂巢文件名的NULL终止Unicode字符串，通过复制蜂巢文件名的最后31个字符。主要用于调试目的。
 *
 * @param[in,out] BaseBlock
 * 指向基础块头的指针，蜂巢文件名将复制到此处。
 *
 * @param[in] FileName
 * 指向包含蜂巢文件名的Unicode字符串结构的指针。如果此参数为NULL，基础块将不会有任何蜂巢文件名。
 */
static
void
HvpInitFileName(
    _Inout_ PHBASE_BLOCK BaseBlock,
    _In_opt_ PCUNICODE_STRING FileName)
{
    ULONG_PTR Offset;
    SIZE_T    Length;

    /* 总是NULL初始化 */
    RtlZeroMemory(BaseBlock->FileName, (HIVE_FILENAME_MAXLEN + 1) * sizeof(WCHAR));

    /* 如果有蜂巢文件名，复制最后31个字符 */
    if (!FileName) return;

    if (FileName->Length / sizeof(WCHAR) <= HIVE_FILENAME_MAXLEN)
    {
        Offset = 0;
        Length = FileName->Length;
    }
    else
    {
        Offset = FileName->Length / sizeof(WCHAR) - HIVE_FILENAME_MAXLEN;
        Length = HIVE_FILENAME_MAXLEN * sizeof(WCHAR);
    }

    RtlCopyMemory(BaseBlock->FileName, FileName->Buffer + Offset, Length);
}

/**
 * @brief
 * 为新创建的内存中蜂巢初始化蜂巢描述符结构。
 *
 * @param[in,out] RegistryHive
 * 指向注册表蜂巢描述符的指针，其内部结构字段将被初始化。
 *
 * @param[in] FileName
 * 指向包含蜂巢文件名的Unicode字符串结构的指针。如果此参数为NULL，基础块将不会有任何蜂巢文件名。
 *
 * @return
 * 如果函数成功创建蜂巢描述符，则返回STATUS_SUCCESS。如果无法分配基础头块，则返回STATUS_NO_MEMORY。
 */
int
CMAPI
HvpCreateHive(
    _Inout_ PHHIVE RegistryHive,
    _In_opt_ PCUNICODE_STRING FileName)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Index;

    /* 分配基础块 */
    BaseBlock = HvpAllocBaseBlockAligned(RegistryHive, FALSE, TAG_CM);
    if (BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    /* 清零基础块 */
    RtlZeroMemory(BaseBlock, RegistryHive->BaseBlockAlloc);

    BaseBlock->Signature = HV_HBLOCK_SIGNATURE;
    BaseBlock->Major = HSYS_MAJOR;
    BaseBlock->Minor = HSYS_MINOR;
    BaseBlock->Type = HFILE_TYPE_PRIMARY;
    BaseBlock->Format = HBASE_FORMAT_MEMORY;
    BaseBlock->Cluster = 1;
    BaseBlock->RootCell = HCELL_NIL;
    BaseBlock->Length = 0;
    BaseBlock->Sequence1 = 1;
    BaseBlock->Sequence2 = 1;
    BaseBlock->TimeStamp.QuadPart = 0ULL;

    /*
     * 由于蜂巢目前仅存在于内存中，因此无需计算校验和。
     */
    BaseBlock->CheckSum = 0;

    /* 设置默认启动类型 */
    BaseBlock->BootType = HBOOT_TYPE_REGULAR;

    /* 设置蜂巢数据 */
    RegistryHive->BaseBlock = BaseBlock;
    RegistryHive->Version = BaseBlock->Minor; // == HSYS_MINOR

    for (Index = 0; Index < 24; Index++)
    {
        RegistryHive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
        RegistryHive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
    }

    HvpInitFileName(BaseBlock, FileName);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * 从已经加载到内存中的注册表蜂巢初始化蜂巢描述符。蜂巢数据被复制并准备进行读/写访问。
 *
 * @param[in] Hive
 * 指向注册表蜂巢描述符的指针，其内部结构字段将从已经加载到内存中的蜂巢数据中初始化。
 *
 * @param[in] ChunkBase
 * 指向包含注册表头数据的有效基础块头的指针，用于初始化。
 *
 * @param[in] FileName
 * 指向包含蜂巢文件名的Unicode字符串结构的指针。如果此参数为NULL，基础块将不会有任何蜂巢文件名。
 *
 * @return
 * 如果函数成功初始化蜂巢描述符，则返回STATUS_SUCCESS。如果基础块头包含无效的头数据，则返回STATUS_REGISTRY_CORRUPT。如果无法分配内存用于注册表内容，则返回STATUS_NO_MEMORY。
 */
int
CMAPI
HvpInitializeMemoryHive(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK ChunkBase,
    _In_opt_ PCUNICODE_STRING FileName)
{
    SIZE_T BlockIndex;
    PHBIN Bin, NewBin;
    ULONG i;
    ULONG BitmapSize;
    PULONG BitmapBuffer;
    SIZE_T ChunkSize;

    ChunkSize = ChunkBase->Length;
    DPRINT("ChunkSize: %zx\n", ChunkSize);

    if (ChunkSize < sizeof(HBASE_BLOCK) ||
        !HvpVerifyHiveHeader(ChunkBase, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("Registry is corrupt: ChunkSize 0x%zx < sizeof(HBASE_BLOCK) 0x%zx, "
                "or HvpVerifyHiveHeader() failed\n", ChunkSize, sizeof(HBASE_BLOCK));
        return STATUS_REGISTRY_CORRUPT;
    }

    /* 分配基础块 */
    Hive->BaseBlock = HvpAllocBaseBlockAligned(Hive, FALSE, TAG_CM);
    if (Hive->BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Hive->BaseBlock, ChunkBase, sizeof(HBASE_BLOCK));

    /* 设置蜂巢数据 */
    Hive->Version = ChunkBase->Minor;

    /*
     * 从内存块构建块列表并复制数据。
     */

    Hive->Storage[Stable].Length = (ULONG)(ChunkSize / HBLOCK_SIZE);
    Hive->Storage[Stable].BlockList =
        static_cast<PHMAP_ENTRY>(Hive->Allocate(Hive->Storage[Stable].Length *
                       sizeof(HMAP_ENTRY), FALSE, TAG_CM));
    if (Hive->Storage[Stable].BlockList == NULL)
    {
        DPRINT1("Allocating block list failed\n");
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    for (BlockIndex = 0; BlockIndex < Hive->Storage[Stable].Length; )
    {
        Bin = reinterpret_cast<PHBIN>(reinterpret_cast<char*>(ChunkBase) + (BlockIndex + 1) * HBLOCK_SIZE);
        if (Bin->Signature != HV_HBIN_SIGNATURE ||
           (Bin->Size % HBLOCK_SIZE) != 0 ||
           (Bin->FileOffset / HBLOCK_SIZE) != BlockIndex)
        {
            /*
             * Bin已损坏，但幸运的是签名、大小或偏移量顺序错误。对于签名，很明显我们要做什么，对于偏移量，我们将bin重新定位到原来的位置，对于大小，我们将它设置为一个块大小，因为技术上蜂巢bin本身就是一个块大小以容纳单元格。
             */
            if (!CmIsSelfHealEnabled(FALSE))
            {
                DPRINT1("Invalid bin at BlockIndex %lu, Signature 0x%x, Size 0x%x. Self-heal not possible!\n",
                    (unsigned long)BlockIndex, (unsigned)Bin->Signature, (unsigned)Bin->Size);
                Hive->Free(Hive->Storage[Stable].BlockList, 0);
                Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
                return STATUS_REGISTRY_CORRUPT;
            }

            /* 修复这个bin */
            Bin->Signature = HV_HBIN_SIGNATURE;
            Bin->Size = HBLOCK_SIZE;
            Bin->FileOffset = BlockIndex * HBLOCK_SIZE;
            ChunkBase->BootType |= HBOOT_TYPE_SELF_HEAL;
            DPRINT1("Bin at index %lu is corrupt and it has been repaired!\n", (unsigned long)BlockIndex);
        }

        NewBin = static_cast<PHBIN>(Hive->Allocate(Bin->Size, TRUE, TAG_CM));
        if (NewBin == NULL)
        {
            Hive->Free(Hive->Storage[Stable].BlockList, 0);
            Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
            return STATUS_NO_MEMORY;
        }

        Hive->Storage[Stable].BlockList[BlockIndex].BinAddress = reinterpret_cast<ULONG_PTR>(NewBin);
        Hive->Storage[Stable].BlockList[BlockIndex].BlockAddress = reinterpret_cast<ULONG_PTR>(NewBin);

        RtlCopyMemory(NewBin, Bin, Bin->Size);

        if (Bin->Size > HBLOCK_SIZE)
        {
            for (i = 1; i < Bin->Size / HBLOCK_SIZE; i++)
            {
                Hive->Storage[Stable].BlockList[BlockIndex + i].BinAddress = reinterpret_cast<ULONG_PTR>(NewBin);
                Hive->Storage[Stable].BlockList[BlockIndex + i].BlockAddress =
                    (reinterpret_cast<ULONG_PTR>(NewBin) + (i * HBLOCK_SIZE));
            }
        }

        BlockIndex += Bin->Size / HBLOCK_SIZE;
    }

    if (!NT_SUCCESS(HvpCreateHiveFreeCellList(Hive)))
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    BitmapSize = ROUND_UP(Hive->Storage[Stable].Length,
                          sizeof(ULONG) * 8) / 8;
    BitmapBuffer = static_cast<PULONG>(Hive->Allocate(BitmapSize, TRUE, TAG_CM));
    if (BitmapBuffer == NULL)
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
    RtlClearAllBits(&Hive->DirtyVector);

    /*
     * 标记整个蜂巢为脏。
     * 确实，如果我们加载了主蜂巢的替代变体（例如SYSTEM.ALT），因为FreeLdr无法加载主SYSTEM蜂巢，由于损坏，并且用LOG修复它也没有帮助。
     */
    if (ChunkBase->BootRecover == HBOOT_BOOT_RECOVERED_BY_ALTERNATE_HIVE)
    {
        RtlSetAllBits(&Hive->DirtyVector);
        Hive->DirtyCount = Hive->DirtyVector.SizeOfBitMap;
    }

    HvpInitFileName(Hive->BaseBlock, FileName);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Initialize a hive from a file by reading bins block-by-block.
 *
 * Unlike HvpInitializeMemoryHive which expects the entire hive in memory,
 * this function reads each bin from disk individually, avoiding the need
 * to allocate a buffer for the entire file.
 */
static int
HvpInitializeFileHive(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK BaseBlock,
    _In_opt_ PCUNICODE_STRING FileName)
{
    SIZE_T BlockIndex;
    PHBIN Bin;
    ULONG i;
    ULONG BitmapSize;
    PULONG BitmapBuffer;
    ULONG FileOffset;
    BOOLEAN Success;

    Hive->BaseBlock = BaseBlock;
    Hive->Version = BaseBlock->Minor;

    Hive->Storage[Stable].Length = BaseBlock->Length / HBLOCK_SIZE;
    Hive->Storage[Stable].BlockList =
        static_cast<PHMAP_ENTRY>(Hive->Allocate(Hive->Storage[Stable].Length *
                       sizeof(HMAP_ENTRY), FALSE, TAG_CM));
    if (Hive->Storage[Stable].BlockList == NULL)
    {
        DPRINT1("Allocating block list failed\n");
        return STATUS_NO_MEMORY;
    }

    for (BlockIndex = 0; BlockIndex < Hive->Storage[Stable].Length; )
    {
        FileOffset = (ULONG)((BlockIndex + 1) * HBLOCK_SIZE);

        Bin = static_cast<PHBIN>(Hive->Allocate(HBLOCK_SIZE, TRUE, TAG_CM));
        if (Bin == NULL)
        {
            HvpFreeHiveBins(Hive);
            return STATUS_NO_MEMORY;
        }

        Success = Hive->FileRead(Hive,
                                 HFILE_TYPE_PRIMARY,
                                 &FileOffset,
                                 Bin,
                                 HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to read bin header at block %lu\n", (unsigned long)BlockIndex);
            Hive->Free(Bin, HBLOCK_SIZE);
            HvpFreeHiveBins(Hive);
            return STATUS_REGISTRY_CORRUPT;
        }

        if (Bin->Signature != HV_HBIN_SIGNATURE ||
           (Bin->Size % HBLOCK_SIZE) != 0 ||
           (Bin->Size == 0) ||
           (Bin->FileOffset / HBLOCK_SIZE) != BlockIndex)
        {
            if (!CmIsSelfHealEnabled(FALSE))
            {
                DPRINT1("Invalid bin at BlockIndex %lu\n", (unsigned long)BlockIndex);
                Hive->Free(Bin, HBLOCK_SIZE);
                HvpFreeHiveBins(Hive);
                return STATUS_REGISTRY_CORRUPT;
            }

            Bin->Signature = HV_HBIN_SIGNATURE;
            Bin->Size = HBLOCK_SIZE;
            Bin->FileOffset = (ULONG)(BlockIndex * HBLOCK_SIZE);
            BaseBlock->BootType |= HBOOT_TYPE_SELF_HEAL;
            DPRINT1("Bin at index %lu is corrupt and has been repaired\n", (unsigned long)BlockIndex);
        }

        if (Bin->Size > HBLOCK_SIZE)
        {
            PHBIN NewBin = static_cast<PHBIN>(Hive->Allocate(Bin->Size, TRUE, TAG_CM));
            if (NewBin == NULL)
            {
                Hive->Free(Bin, HBLOCK_SIZE);
                HvpFreeHiveBins(Hive);
                return STATUS_NO_MEMORY;
            }

            RtlCopyMemory(NewBin, Bin, HBLOCK_SIZE);
            Hive->Free(Bin, HBLOCK_SIZE);
            Bin = NewBin;

            FileOffset = (ULONG)((BlockIndex + 1) * HBLOCK_SIZE + HBLOCK_SIZE);
            Success = Hive->FileRead(Hive,
                                     HFILE_TYPE_PRIMARY,
                                     &FileOffset,
                                     reinterpret_cast<char*>(Bin) + HBLOCK_SIZE,
                                     Bin->Size - HBLOCK_SIZE);
            if (!Success)
            {
                DPRINT1("Failed to read bin body at block %lu\n", (unsigned long)BlockIndex);
                Hive->Free(Bin, Bin->Size);
                HvpFreeHiveBins(Hive);
                return STATUS_REGISTRY_CORRUPT;
            }
        }

        Hive->Storage[Stable].BlockList[BlockIndex].BinAddress = reinterpret_cast<ULONG_PTR>(Bin);
        Hive->Storage[Stable].BlockList[BlockIndex].BlockAddress = reinterpret_cast<ULONG_PTR>(Bin);

        if (Bin->Size > HBLOCK_SIZE)
        {
            for (i = 1; i < Bin->Size / HBLOCK_SIZE; i++)
            {
                Hive->Storage[Stable].BlockList[BlockIndex + i].BinAddress = reinterpret_cast<ULONG_PTR>(Bin);
                Hive->Storage[Stable].BlockList[BlockIndex + i].BlockAddress =
                    (reinterpret_cast<ULONG_PTR>(Bin) + (i * HBLOCK_SIZE));
            }
        }

        BlockIndex += Bin->Size / HBLOCK_SIZE;
    }

    if (!NT_SUCCESS(HvpCreateHiveFreeCellList(Hive)))
    {
        HvpFreeHiveBins(Hive);
        return STATUS_NO_MEMORY;
    }

    BitmapSize = ROUND_UP(Hive->Storage[Stable].Length,
                          sizeof(ULONG) * 8) / 8;
    BitmapBuffer = static_cast<PULONG>(Hive->Allocate(BitmapSize, TRUE, TAG_CM));
    if (BitmapBuffer == NULL)
    {
        HvpFreeHiveBins(Hive);
        return STATUS_NO_MEMORY;
    }

    RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
    RtlClearAllBits(&Hive->DirtyVector);

    if (BaseBlock->BootRecover == HBOOT_BOOT_RECOVERED_BY_ALTERNATE_HIVE)
    {
        RtlSetAllBits(&Hive->DirtyVector);
        Hive->DirtyCount = Hive->DirtyVector.SizeOfBitMap;
    }

    HvpInitFileName(Hive->BaseBlock, FileName);

    return STATUS_SUCCESS;
}


/**
 * @brief
 * 初始化已经加载到内存中的蜂巢的蜂巢描述符。该描述符表示蜂巢为“平坦”的，即数据和属性只能读取而不能写入。
 *
 * @param[in] Hive
 * 指向注册表蜂巢描述符的指针，其内部结构字段将从已经加载到内存中的蜂巢数据中初始化。这样的蜂巢描述符将成为只读且平坦的。
 *
 * @param[in] ChunkBase
 * 指向包含注册表头数据的有效基础块头的指针，用于初始化。
 *
 * @return
 * 如果函数成功初始化平坦蜂巢描述符，则返回STATUS_SUCCESS。如果基础块头包含无效的头数据，则返回STATUS_REGISTRY_CORRUPT。
 */
int
CMAPI
HvpInitializeFlatHive(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK ChunkBase)
{
    if (!HvpVerifyHiveHeader(ChunkBase, HFILE_TYPE_PRIMARY))
        return STATUS_REGISTRY_CORRUPT;

    /* 设置蜂巢数据 */
    Hive->BaseBlock = ChunkBase;
    Hive->Version = ChunkBase->Minor;
    Hive->Flat = TRUE;
    Hive->ReadOnly = TRUE;

    Hive->StorageTypeCount = 1;

    /* 设置默认启动类型 */
    ChunkBase->BootType = HBOOT_TYPE_REGULAR;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * 从存储在物理后备存储中的主蜂巢文件中获取基础块蜂巢头。如果无法正常获取蜂巢头，此函数可能会触发自愈警告。详见返回和备注部分。
 *
 * @param[in] Hive
 * 指向注册表蜂巢描述符的指针，指向正在加载的主蜂巢。该描述符用于从该蜂巢中获取蜂巢头块。
 *
 * @param[in,out] HiveBaseBlock
 * 由函数返回的指针，包含从主蜂巢文件中获取的蜂巢头基础块缓冲区。此参数不能为NULL！
 *
 * @param[in,out] TimeStamp
 * 由函数返回的指针，包含注册表蜂巢文件在创建或修改时刻的时间戳。此参数不能为NULL！
 *
 * @return
 * 该函数返回一个结果指示器。即，如果蜂巢头成功获取，则返回HiveSuccess。
 * 如果无法分配蜂巢基础块，则返回NoMemory。
 * 如果读取的蜂巢文件实际上不是蜂巢，则返回NotHive。
 * 如果需要恢复头，则返回RecoverHeader。
 * 如果需要恢复数据，则返回RecoverData。
 *
 * @remarks
 * RecoverHeader和RecoverData是状态指示器，如果蜂巢头无法正常获取，则会触发自愈过程。
 * RecoverHeader表示蜂巢的基础块头已损坏，需要恢复，而RecoverData表示注册表数据已损坏。
 * 后一个状态指示器不如前一个严重，因为系统可以处理数据丢失。
 */
RESULT
CMAPI
HvpGetHiveHeader(
    _In_ PHHIVE Hive,
    _Inout_ PHBASE_BLOCK *HiveBaseBlock,
    _Inout_ PLARGE_INTEGER TimeStamp)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Result;
    ULONG FileOffset;
    PHBIN FirstBin;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* 假设失败并分配基础块 */
    *HiveBaseBlock = NULL;
    BaseBlock = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!BaseBlock)
    {
        DPRINT1("Failed to allocate an aligned base block buffer\n");
        return NoMemory;
    }

    /* 清零基础块 */
    RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

    /* 现在从磁盘读取 */
    FileOffset = 0;
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &FileOffset,
                            BaseBlock,
                            Hive->Cluster * HSECTOR_SIZE);
    if (!Result)
    {
        /*
         * 不要假设蜂巢完全被破坏，而是尝试读取第一个bin蜂巢的第一个块。
         * 这样我们可以确认可以恢复这个蜂巢。
         */
        FileOffset = HBLOCK_SIZE;
        Result = Hive->FileRead(Hive,
                                HFILE_TYPE_PRIMARY,
                                &FileOffset,
                                static_cast<void*>(BaseBlock),
                                Hive->Cluster * HSECTOR_SIZE);
        if (!Result)
        {
            DPRINT1("Failed to read the first block of the first bin hive (hive too corrupt)\n");
            Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
            return NotHive;
        }

        /*
         * 将我们得到的缓冲区转换为蜂巢bin。检查偏移位置是否正确（即其偏移量必须为0，因为它是第一个bin）并且它应该有一个合理的签名。
         */
        FirstBin = reinterpret_cast<PHBIN>(BaseBlock);
        if (FirstBin->Signature != HV_HBIN_SIGNATURE ||
            FirstBin->FileOffset != 0)
        {
            DPRINT1("Failed to read the first block of the first bin hive (hive too corrupt)\n");
            Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
            return NotHive;
        }

        /*
         * 这个蜂巢还有希望，所以通知调用者这个蜂巢需要可恢复的头。
         */
        *TimeStamp = BaseBlock->TimeStamp;
        DPRINT1("The hive is not fully corrupt, the base block needs to be RECOVERED\n");
        return RecoverHeader;
    }

    /*
     * Check if sequence numbers mismatch — this means the hive data is stale
     * (log was not fully applied) but the header itself may be valid.
     */
    if (BaseBlock->Sequence1 != BaseBlock->Sequence2)
    {
        /* Verify the rest of the header is valid (signature, checksum, etc.) */
        if (BaseBlock->Signature == HV_HBLOCK_SIGNATURE &&
            BaseBlock->Major == HSYS_MAJOR &&
            BaseBlock->Minor >= HSYS_MINOR &&
            BaseBlock->Type == HFILE_TYPE_PRIMARY &&
            BaseBlock->Format == HBASE_FORMAT_MEMORY &&
            BaseBlock->Cluster == 1 &&
            HvpHiveHeaderChecksum(BaseBlock) == BaseBlock->CheckSum)
        {
            DPRINT1("Sequence mismatch detected, data recovery needed\n");
            *HiveBaseBlock = BaseBlock;
            *TimeStamp = BaseBlock->TimeStamp;
            return RecoverData;
        }

        DPRINT1("Sequence mismatch with other header corruption\n");
        *TimeStamp = BaseBlock->TimeStamp;
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return RecoverHeader;
    }

    /* Header is fully valid */
    if (!HvpVerifyHiveHeader(BaseBlock, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("The hive base header block needs to be RECOVERED\n");
        *TimeStamp = BaseBlock->TimeStamp;
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return RecoverHeader;
    }

    /* 返回信息 */
    *HiveBaseBlock = BaseBlock;
    *TimeStamp = BaseBlock->TimeStamp;
    return HiveSuccess;
}

typedef enum _FILE_INFORMATION_CLASS
{
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation,
    FileBothDirectoryInformation,
    FileBasicInformation,
    FileStandardInformation,
    FileInternalInformation,
    FileEaInformation,
    FileAccessInformation,
    FileNameInformation,
    FileRenameInformation,
    FileLinkInformation,
    FileNamesInformation,
    FileDispositionInformation,
    FilePositionInformation,
    FileFullEaInformation,
    FileModeInformation,
    FileAlignmentInformation,
    FileAllInformation,
    FileAllocationInformation,
    FileEndOfFileInformation,
    FileAlternateNameInformation,
    FileStreamInformation,
    FilePipeInformation,
    FilePipeLocalInformation,
    FilePipeRemoteInformation,
    FileMailslotQueryInformation,
    FileMailslotSetInformation,
    FileCompressionInformation,
    FileObjectIdInformation,
    FileCompletionInformation,
    FileMoveClusterInformation,
    FileQuotaInformation,
    FileReparsePointInformation,
    FileNetworkOpenInformation,
    FileAttributeTagInformation,
    FileTrackingInformation,
    FileIdBothDirectoryInformation,
    FileIdFullDirectoryInformation,
    FileValidDataLengthInformation,
    FileShortNameInformation,
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

int

ZwQueryInformationFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_ void* FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass
);

/**
 * @brief
 * 通过查询关联的蜂巢文件的文件大小来计算蜂巢空间大小。
 *
 * @param[in] Hive
 * 指向蜂巢描述符的指针，蜂巢长度大小将在此计算。
 *
 * @return
 * 返回计算的蜂巢大小。
 */
#if 0
ULONG
CMAPI
HvpQueryHiveSize(
    _In_ PHHIVE Hive)
{
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    int Status;
    FILE_STANDARD_INFORMATION FileStandard;
    IO_STATUS_BLOCK IoStatusBlock;
#endif
    ULONG HiveSize = 0;

    /*
     * 查询物理蜂巢文件的文件大小。我们需要这些信息来确保蜂巢的实际大小。
     */
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    Status = ZwQueryInformationFile((reinterpret_cast<PCMHIVE>(Hive))->FileHandles[HFILE_TYPE_PRIMARY],
                                    &IoStatusBlock,
                                    &FileStandard,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQueryInformationFile returned 0x%lx\n", Status);
        return HiveSize;
    }

    /* 现在计算蜂巢大小 */
    HiveSize = FileStandard.EndOfFile.u.LowPart - HBLOCK_SIZE;
#endif
    return HiveSize;
}
#endif
/**
 * @brief 查询蜂巢文件的大小。
 *
 * 该函数查询指定蜂巢文件的大小，并计算蜂巢大小。
 *
 * @param RegistryHive 指向注册表蜂巢结构的指针。
 *
 * @return 返回计算后的蜂巢大小，如果失败则返回0。
 */
ULONG
CMAPI
HvpQueryHiveSize(PHHIVE RegistryHive)
{
    PCMHIVE CmHive = reinterpret_cast<PCMHIVE>(RegistryHive);
    FILE *File = static_cast<FILE*>(CmHive->FileHandles[HFILE_TYPE_PRIMARY]);

    if (File == NULL)
    {
        DPRINT1("File handle is NULL\n");
        return 0;
    }

    // 将文件指针移动到文件末尾
    if (fseek(File, 0, SEEK_END) != 0)
    {
        DPRINT1("Failed to seek to end of file\n");
        return 0;
    }

    // 获取文件大小
    long fileSize = ftell(File);
    if (fileSize == -1)
    {
        DPRINT1("Failed to get file size\n");
        return 0;
    }

    // 计算蜂巢大小
    ULONG HiveSize = (ULONG)fileSize - HBLOCK_SIZE;
    return HiveSize;
}



/**
 * @brief
 * 通过与蜂巢关联的日志文件恢复基础块头。
 *
 * @param[in] Hive
 * 指向与日志文件关联的蜂巢描述符的指针，蜂巢头将从该日志文件中读取。
 *
 * @param[in] TimeStamp
 * 指向时间戳的指针，用于检查提供的时间是否与蜂巢的时间匹配。
 *
 * @param[in,out] BaseBlock
 * 由调用者返回的指针，包含从日志中读取的基础块头。该基础块也可以手动创建。详见备注。
 *
 * @return
 * 如果头正常从日志中获取，则返回HiveSuccess。如果基础块头无法分配，则返回NoMemory。如果自愈模式被禁用且日志无法读取或写入主蜂巢失败，则返回Fail。如果自愈模式继续，则返回SelfHeal。
 *
 * @remarks
 * 当返回SelfHeal时，这表明即使我们手头的日志是损坏的，但我们至少有一个日志，我们唯一的希望是通过手动重建基础头部的片段。
 */
RESULT
CMAPI
HvpRecoverHeaderFromLog(
    _In_ PHHIVE Hive,
    _In_ PLARGE_INTEGER TimeStamp,
    _Inout_ PHBASE_BLOCK *BaseBlock)
{
    BOOLEAN Success;
    PHBASE_BLOCK LogHeader;
    ULONG FileOffset;
    ULONG HiveSize;
    BOOLEAN HeaderResuscitated;

    /*
     * 集群的大小不能超过基础块允许的大小。
     */
    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* 假设我们还没有复苏头 */
    HeaderResuscitated = FALSE;

    /* 为日志头分配对齐的缓冲区 */
    LogHeader = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!LogHeader)
    {
        DPRINT1("Failed to allocate memory for the log header\n");
        return NoMemory;
    }

    /* 清零我们的头缓冲区 */
    RtlZeroMemory(LogHeader, HSECTOR_SIZE);

    /* 从日志中获取基础头 */
    FileOffset = 0;
    Success = Hive->FileRead(Hive,
                             HFILE_TYPE_LOG,
                             &FileOffset,
                             LogHeader,
                             Hive->Cluster * HSECTOR_SIZE);
    if (!Success ||
        !HvpVerifyHiveHeader(LogHeader, HFILE_TYPE_LOG) ||
        TimeStamp->HighPart != LogHeader->TimeStamp.HighPart ||
        TimeStamp->LowPart != LogHeader->TimeStamp.LowPart)
    {
        /*
         * 我们无法从日志中读取基础块头，或者头本身或时间戳无效。检查是否启用了自愈功能。
         */
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log couldn't be read and self-healing mode is disabled\n");
            Hive->Free(LogHeader, Hive->BaseBlockAlloc);
            return Fail;
        }

        /*
         * 确定蜂巢的大小，以便我们可以确定我们要复苏的基础块的长度。
         */
        HiveSize = HvpQueryHiveSize(Hive);
        if (HiveSize == 0)
        {
            DPRINT1("Failed to query the hive size\n");
            Hive->Free(LogHeader, Hive->BaseBlockAlloc);
            return Fail;
        }

        /*
         * 如果我们无法从日志中获取基础头，我们可以通过手动重建头的内部来复苏基础头（假设根单元格不是NIL或损坏的）。CmCheckRegistry在蜂巢初始化和加载后对根单元格是否严重损坏做出最终判断。
         *
         * 有关基础块头复苏的更多信息，请参见https://github.com/msuhanov/regf/blob/master/Windows%20registry%20file%20format%20specification.md#notes-4。
         */
        LogHeader->Signature = HV_HBLOCK_SIGNATURE;
        LogHeader->Sequence1 = 1;
        LogHeader->Sequence2 = 1;
        LogHeader->Cluster = 1;
        LogHeader->Length = HiveSize;
        LogHeader->CheckSum = HvpHiveHeaderChecksum(LogHeader);

        /*
         * 确认我们已经复苏了头。
         */
        HeaderResuscitated = TRUE;
        DPRINT1("Header has been resuscitated, triggering self-heal mode\n");
    }

    /*
     * 在写入蜂巢之前，将此日志头标记为主蜂巢。
     */
    LogHeader->Type = HFILE_TYPE_PRIMARY;

    /*
     * 如果我们没有尝试从日志损坏中恢复头，则必须计算校验和。这在头被复苏时已经完成，所以不要尝试两次。
     */
    if (!HeaderResuscitated)
    {
        LogHeader->CheckSum = HvpHiveHeaderChecksum(LogHeader);
    }

    /* 现在将头写回蜂巢 */
    Success = Hive->FileWrite(Hive,
                              HFILE_TYPE_PRIMARY,
                              &FileOffset,
                              LogHeader,
                              Hive->Cluster * HSECTOR_SIZE);
    if (!Success)
    {
        DPRINT1("Couldn't write the base header to primary hive\n");
        Hive->Free(LogHeader, Hive->BaseBlockAlloc);
        return Fail;
    }

    *BaseBlock = LogHeader;
    return HeaderResuscitated ? SelfHeal : HiveSuccess;
}


/**
 * @brief
 * 通过与蜂巢关联的日志恢复注册表数据。
 *
 * @param[in] Hive
 * 指向与日志文件关联的蜂巢描述符的指针，蜂巢数据将从该日志文件中读取。
 *
 * @param[in] BaseBlock
 * 指向基础块头部的指针。
 *
 * @return
 * 如果数据正常从日志中获取，则返回HiveSuccess。如果自愈功能被禁用且无法从日志中读取数据，或者脏位向量签名无效，或者无法将数据块写入主蜂巢，则返回Fail。如果日志损坏且系统将继续在数据丢失的情况下恢复，则返回SelfHeal。
 */
RESULT
CMAPI
HvpRecoverDataFromLog(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK BaseBlock)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LogIndex;
    ULONG StorageLength;
    unsigned char DirtyVector[HSECTOR_SIZE];
    unsigned char Buffer[HBLOCK_SIZE];

    /* 从日志中读取脏数据 */
    FileOffset = HV_LOG_HEADER_SIZE;
    Success = Hive->FileRead(Hive,
                             HFILE_TYPE_LOG,
                             &FileOffset,
                             DirtyVector,
                             HSECTOR_SIZE);
    if (!Success)
    {
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log couldn't be read and self-healing mode is disabled\n");
            return Fail;
        }

        /*
         * 在无法从日志中读取脏数据的情况下，触发自愈模式并继续。最坏的情况是数据丢失。
         */
        DPRINT1("Triggering self-heal mode, DATA LOSS IS IMMINENT\n");
        return SelfHeal;
    }

    /* 检查脏位向量 */
    if (*reinterpret_cast<PULONG>(DirtyVector) != HV_LOG_DIRTY_SIGNATURE)
    {
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log's dirty vector signature is not valid\n");
            return Fail;
        }

        /*
         * 如果脏位向量签名无效，触发自愈模式。签名之后的任何数据逻辑上也是无效的。
         */
        DPRINT1("Triggering self-heal mode, DATA LOSS IS IMMINENT\n");
        return SelfHeal;
    }

    /* 现在逐个读取数据并将其写回蜂巢 */
    LogIndex = 0;
    StorageLength = BaseBlock->Length / HBLOCK_SIZE;
    for (BlockIndex = 0; BlockIndex < StorageLength; BlockIndex++)
    {
        /* 如果该块不脏，则跳过并继续下一个块 */
        if (DirtyVector[BlockIndex + sizeof(HV_LOG_DIRTY_SIGNATURE)] != HV_LOG_DIRTY_BLOCK)
        {
            continue;
        }

        FileOffset = HSECTOR_SIZE + HSECTOR_SIZE + LogIndex * HBLOCK_SIZE;
        Success = Hive->FileRead(Hive,
                                 HFILE_TYPE_LOG,
                                 &FileOffset,
                                 Buffer,
                                 HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to read the dirty block (index %lu)\n", BlockIndex);
            return Fail;
        }

        FileOffset = HBLOCK_SIZE + BlockIndex * HBLOCK_SIZE;
        Success = Hive->FileWrite(Hive,
                                  HFILE_TYPE_PRIMARY,
                                  &FileOffset,
                                  Buffer,
                                  HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write dirty block to hive (index %lu)\n", BlockIndex);
            return Fail;
        }

        /* 增加日志中的索引，继续下一步 */
        LogIndex++;
    }

    return HiveSuccess;
}

/**
 * @brief
 * 从物理hive文件中加载注册表hive，该文件位于物理备份存储中。基础块和注册表数据从所述物理hive文件中读取。
 * 如果hive加载无法正常进行，此函数可以执行注册表恢复。
 *
 * @param[in] Hive
 * 指向hive描述符的指针，所述hive将从物理hive文件中加载。
 *
 * @param[in] FileName
 * 指向包含要复制的hive文件名的NULL终止Unicode字符串结构的指针。
 *
 * @return
 * 如果hive成功加载，则返回STATUS_SUCCESS。
 * 如果没有足够的内存资源来满足注册表操作和/或请求，则返回STATUS_INSUFFICIENT_RESOURCES。
 * 如果hive实际上不是hive文件，则返回STATUS_NOT_REGISTRY_FILE。
 * 如果hive之前受损且无法恢复，因为没有日志或自愈功能被禁用，则返回STATUS_REGISTRY_CORRUPT。
 * 如果hive已恢复，则返回STATUS_REGISTRY_RECOVERED。在hive完全加载后，需要最终刷新注册表。
 */
int
CMAPI
HvLoadHive(
    _In_ PHHIVE Hive,
    _In_opt_ PCUNICODE_STRING FileName)
{
    int Status;
    BOOLEAN Success;
    PHBASE_BLOCK BaseBlock = NULL;

    ULONG Result, Result2;

    LARGE_INTEGER TimeStamp;
    BOOLEAN HiveSelfHeal = FALSE;

    DPRINT1("Loading hive from file %wZ\n", FileName);

    /* 获取hive头 */
    Result = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);
    DPRINT1("HvpGetHiveHeader Result: %d\n", Result);
    switch (Result)
    {
        /* 内存不足 */
        case NoMemory:
        {
            /* 失败 */
            DPRINT1("There's no enough memory to get the header\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* 不是hive */
        case NotHive:
        {
            /* 失败 */
            DPRINT1("The hive is not an actual registry hive file\n");
            return STATUS_NOT_REGISTRY_FILE;
        }

        /* Hive数据需要修复 (header valid, sequence mismatch) */
        case RecoverData:
        {
            DPRINT1("Sequence mismatch detected, recovering data from log...\n");

            /* Check if this hive has a log for data recovery */
            if (!Hive->Log)
            {
                DPRINT1("The hive has no log for data recovery\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* Recover data from the log — header is already valid */
            Result2 = HvpRecoverDataFromLog(Hive, BaseBlock);
            if (Result2 == Fail)
            {
                DPRINT1("Failed to recover data from log, falling back to full recovery\n");

                /* Data recovery failed, try full header+data recovery */
                Result2 = HvpRecoverHeaderFromLog(Hive, &TimeStamp, &BaseBlock);
                if (Result2 == Fail)
                {
                    DPRINT1("Full recovery also failed\n");
                    return STATUS_REGISTRY_CORRUPT;
                }

                if (Result2 == SelfHeal)
                    HiveSelfHeal = TRUE;
            }

            if (Result2 == SelfHeal)
                HiveSelfHeal = TRUE;

            break;
        }

        /* Hive头需要修复 */
        case RecoverHeader:
        {
            /* 检查此hive是否有日志可供头恢复 */
            if (!Hive->Log)
            {
                DPRINT1("The hive has no log for header recovery\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* 头需要恢复，所以进行恢复 */
            DPRINT1("Attempting to heal the header...\n");
            Result2 = HvpRecoverHeaderFromLog(Hive, &TimeStamp, &BaseBlock);
            if (Result2 == NoMemory)
            {
                DPRINT1("There's no enough memory to recover header from log\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* 失败了吗？ */
            if (Result2 == Fail)
            {
                DPRINT1("Failed to recover the hive header\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* 触发自愈模式了吗？ */
            if (Result2 == SelfHeal)
            {
                HiveSelfHeal = TRUE;
            }

            /* 现在恢复数据 */
            Result2 = HvpRecoverDataFromLog(Hive, BaseBlock);
            if (Result2 == Fail)
            {
                DPRINT1("Failed to recover the hive data\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* 如果之前没有标记自愈模式，则标记为自愈 */
            if ((Result2 == SelfHeal) && (!HiveSelfHeal))
            {
                HiveSelfHeal = TRUE;
            }

            break;
        }
    }

    /* 设置启动类型 */
    BaseBlock->BootType = HiveSelfHeal ? HBOOT_TYPE_SELF_HEAL : HBOOT_TYPE_REGULAR;

    /* 设置hive数据 */
    Hive->BaseBlock = BaseBlock;
    Hive->Version = BaseBlock->Minor;

    /* Load hive bins block-by-block from disk */
    Status = HvpInitializeFileHive(Hive, BaseBlock, FileName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize file hive\n");
        return Status;
    }

    /*
     * 如果我们对要从文件加载的hive进行了某种恢复，告诉调用者我们已恢复它。调用者负责稍后刷新数据。
     */
    return (Result == RecoverHeader || Result == RecoverData) ? STATUS_REGISTRY_RECOVERED : STATUS_SUCCESS;
}

/**
 * @brief
 * 初始化注册表hive。它分配一个hive描述符并根据调用者选择的类型设置hive类型。
 *
 * @param[in,out] RegistryHive
 * 指向要初始化的hive描述符的指针。
 *
 * @param[in] OperationType
 * 用于hive初始化的操作类型。有关更多信息，请参见备注。
 *
 * @param[in] HiveFlags
 * 一个hive标志。该标志用于确定必须对hive采取的行动或必须考虑的方面。有关更多信息，请参见备注。
 *
 * @param[in] FileType
 * Hive文件类型。对于新初始化的hive，您可以从三种不同的类型中选择：
 *
 * HFILE_TYPE_PRIMARY - 将hive初始化为系统的主hive。
 *
 * HFILE_TYPE_LOG - 新创建的hive是一个hive日志。日志本身不存在，但与它们关联的主hive一起存在。Hive描述符的Log字段成员设置为TRUE。
 *
 * HFILE_TYPE_EXTERNAL - 新创建的hive是一个便携式hive，可以用于不同的机器，不像主hive。
 *
 * HFILE_TYPE_ALTERNATE - 新创建的hive是一个备用hive。从技术上讲，它与主hive相同（磁盘上的注册表头表示为HFILE_TYPE_PRIMARY），目的是作为备份hive。Hive描述符的Alternate字段设置为TRUE。只有SYSTEM hive有一个备份备用hive。
 *
 * @param[in] HiveData
 * 指向hive数据的任意指针。通常，这些数据是由调用此函数的调用者提供的一个hive基础块。
 *
 * @param[in] Allocate
 * 指向描述此hive主要分配例程的ALLOCATE_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] Free
 * 指向描述此hive主要内存释放例程的FREE_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] FileSetSize
 * 指向描述此hive文件设置大小例程的FILE_SET_SIZE_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] FileWrite
 * 指向描述此hive文件写入例程的FILE_WRITE_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] FileRead
 * 指向描述此hive文件读取例程的FILE_READ_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] FileFlush
 * 指向描述此hive文件刷新例程的FILE_FLUSH_ROUTINE函数的指针。此参数可以为NULL。
 *
 * @param[in] Cluster
 * 要设置的注册表hive集群。通常，此值设置为1。
 *
 * @param[in] FileName
 * 指向包含hive文件名的NULL终止Unicode字符串结构的指针。此参数可以为NULL。
 *
 * @return
 * 如果函数成功初始化hive，则返回STATUS_SUCCESS。如果hive之前受损并已恢复，则返回STATUS_REGISTRY_RECOVERED。如果恢复数据的注册表hive写入/刷新失败，则返回STATUS_REGISTRY_IO_FAILED。如果提交的OperationType参数无效，则返回STATUS_INVALID_PARAMETER。否则返回失败的int代码。
 *
 * @remarks
 * OperationType参数影响hive应如何初始化。以下是支持的操作类型：
 *
 * HINIT_CREATE -- 创建一个新的hive。
 *
 * HINIT_MEMORY -- 从内存中初始化已存在的注册表hive。hive数据从内存中加载的hive复制，并用于读/写访问。
 *
 * HINIT_FLAT -- 初始化一个只能读取而不能写入的平面注册表hive。单元格总是在平面hive中分配。
 *
 * HINIT_FILE -- 从系统的物理备份存储中的hive文件初始化hive。在这种情况下，如果从物理hive文件读取的数据已损坏，函数将执行自愈和复苏程序。
 *
 * HINIT_MEMORY_INPLACE -- 此操作类型类似于HINIT_FLAT，区别在于hive从内存中的hive数据初始化。hive只能读取而不能写入。
 *
 * HINIT_MAPFILE -- 从系统的物理备份存储中的hive文件初始化hive。与HINIT_FILE不同，初始化的hive不备份到内存中的分页池，而是通过映射视图。
 *
 * 除了操作类型，hive标志也影响新初始化hive的方面。以下是支持的hive标志：
 *
 * HIVE_VOLATILE -- 告诉函数此hive将是易失性的，即存储在hive空间中的数据仅存在于系统的易失性内存（RAM）中，并且数据将在系统关闭时被擦除。
 *
 * HIVE_NOLAZYFLUSH -- 告诉函数此hive不得进行延迟刷新。
 */
int
CMAPI
HvInitialize(
    _Inout_ PHHIVE RegistryHive,
    _In_  ULONG OperationType,
    _In_  ULONG HiveFlags,
    _In_  ULONG FileType,
    _In_opt_ void* HiveData,
    _In_opt_ PALLOCATE_ROUTINE Allocate,
    _In_opt_ PFREE_ROUTINE Free,
    _In_opt_ PFILE_SET_SIZE_ROUTINE FileSetSize,
    _In_opt_ PFILE_WRITE_ROUTINE FileWrite,
    _In_opt_ PFILE_READ_ROUTINE FileRead,
    _In_opt_ PFILE_FLUSH_ROUTINE FileFlush,
    _In_ ULONG Cluster,
    _In_opt_ PCUNICODE_STRING FileName)
{
    int Status;
    PHHIVE Hive = RegistryHive;

    /*
     * 创建一个新的hive结构，该结构将保存所有维护数据。
     */

    RtlZeroMemory(Hive, sizeof(HHIVE));
    Hive->Signature = HV_HHIVE_SIGNATURE;

    Hive->Allocate = Allocate;
    Hive->Free = Free;
    Hive->FileSetSize = FileSetSize;
    Hive->FileWrite = FileWrite;
    Hive->FileRead = FileRead;
    Hive->FileFlush = FileFlush;

    Hive->RefreshCount = 0;
    Hive->StorageTypeCount = HTYPE_COUNT;
    Hive->Cluster = Cluster;
    Hive->BaseBlockAlloc = sizeof(HBASE_BLOCK); // == HBLOCK_SIZE

    Hive->Version = HSYS_MINOR;
    Hive->Log = (FileType == HFILE_TYPE_LOG);
    Hive->Alternate = (FileType == HFILE_TYPE_ALTERNATE);
    Hive->HiveFlags = HiveFlags & ~HIVE_NOLAZYFLUSH;

    // TODO: CellRoutines指向不同的回调，具体取决于OperationType。
    Hive->GetCellRoutine = HvpGetCellData;
    Hive->ReleaseCellRoutine = NULL;

    DPRINT1("HvInitialize: OperationType = %lu\n", OperationType);

    switch (OperationType)
    {
        case HINIT_CREATE:
        {
            /* 创建一个新的hive */
            Status = HvpCreateHive(Hive, FileName);
            break;
        }

        case HINIT_MEMORY:
        {
            /* 从内存中初始化hive */
            Status = HvpInitializeMemoryHive(Hive, static_cast<PHBASE_BLOCK>(HiveData), FileName);
            break;
        }

        case HINIT_FLAT:
        {
            /* 初始化平面只读hive */
            Status = HvpInitializeFlatHive(Hive, static_cast<PHBASE_BLOCK>(HiveData));
            break;
        }

        case HINIT_FILE:
        {
            /* 从备份存储中的物理文件加载hive */
            Status = HvLoadHive(Hive, FileName);
            DPRINT1("HvInitialize: HvLoadHive returned %lu\n", Status);
            if ((Status != STATUS_SUCCESS) &&
                (Status != STATUS_REGISTRY_RECOVERED))
            {
                /* 不可恢复的故障 */
                DPRINT1("Registry hive couldn't be initialized, it's corrupt (hive 0x%p)\n", Hive);
                return Status;
            }

            /*
             * 检查我们是否已恢复此hive。我们有责任随后将主hive刷新回备份存储。
             */
            if (Status == STATUS_REGISTRY_RECOVERED)
            {
                if (!HvSyncHiveFromRecover(Hive))
                {
                    DPRINT1("Fail to write healthy data back to hive\n");
                    return STATUS_REGISTRY_IO_FAILED;
                }

                /* Clear dirty state after successful recovery */
                RtlClearAllBits(&Hive->DirtyVector);
                Hive->DirtyCount = 0;
                Hive->LogSize = 0;

                /*
                 * 将状态代码伪装为成功。STATUS_REGISTRY_RECOVERED不是失败代码，但也不是STATUS_SUCCESS，因此调用者认为我们未能完成工作。
                 */
                Status = STATUS_SUCCESS;
            }
            break;
        }

        case HINIT_MEMORY_INPLACE:
        {
            /* 在内存中原地初始化hive */
            // Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
            // break;
            DPRINT1("HINIT_MEMORY_INPLACE is UNIMPLEMENTED\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        case HINIT_MAPFILE:
        {
            /* 从映射文件初始化hive */
            DPRINT1("HINIT_MAPFILE is UNIMPLEMENTED\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        default:
        {
            DPRINT1("Invalid operation type (OperationType = %lu)\n", OperationType);
            return STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}


/**
 * @brief
 * 释放给定注册表蜂巢描述符中所有存储桶、脏位向量和基础块。
 *
 * @param[in] RegistryHive
 * 指向一个蜂巢描述符的指针，其所有数据将被释放。
 */
void
CMAPI
HvFree(
    _In_ PHHIVE RegistryHive)
{
    if (!RegistryHive->ReadOnly)
    {
        /* 释放蜂巢位图 */
        if (RegistryHive->DirtyVector.Buffer)
        {
            RegistryHive->Free(RegistryHive->DirtyVector.Buffer, 0);
        }

        HvpFreeHiveBins(RegistryHive);

        /* 释放基础块 */
        if (RegistryHive->BaseBlock)
        {
            RegistryHive->Free(RegistryHive->BaseBlock, RegistryHive->BaseBlockAlloc);
            RegistryHive->BaseBlock = NULL;
        }
    }
}


/* EOF */
