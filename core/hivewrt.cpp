/*
 * PROJECT:   NTREG Kernel
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

// #define NDEBUG
#include "cmlib.hpp"

#include "debug.hpp"

/* DECLARATIONS *************************************************************/


BOOLEAN
IoSetThreadHardErrorMode(
    _In_ BOOLEAN HardErrorEnabled);


/* GLOBALS ******************************************************************/

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief 验证蜂巢基础头
 *
 * 这个函数用于验证蜂巢基础头的有效性。
 *
 * @param RegistryHive 蜂巢指针
 */
static
void
HvpValidateBaseHeader(
    _In_ PHHIVE RegistryHive)
{
    PHBASE_BLOCK BaseBlock;

    // 缓存基础块并验证
    BaseBlock = RegistryHive->BaseBlock;
    ASSERT(BaseBlock->Signature == HV_HBLOCK_SIGNATURE);  // 验证签名
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);  // 验证格式
    ASSERT(BaseBlock->Major == HSYS_MAJOR);  // 验证主要版本
}


/**
 * @brief 写入蜂巢日志
 *
 * 这个函数用于在蜂巢同步操作期间以事务方式将脏数据写入蜂巢日志文件。日志文件用于内核/引导加载程序对损坏的主蜂巢执行恢复操作。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果日志事务写入成功，返回TRUE，否则返回FALSE
 */
static
BOOLEAN
CMAPI
HvpWriteLog(
    _In_ PHHIVE RegistryHive)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LastIndex;
    void* Block;
    UINT32 BitmapSize, BufferSize;
    unsigned char* HeaderBuffer;
    unsigned char* Ptr;

    // 蜂巢日志必须是可写的且存储有效
    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->BaseBlock->Length ==
           RegistryHive->Storage[Stable].Length * HBLOCK_SIZE);

    // 验证基础头
    HvpValidateBaseHeader(RegistryHive);

    // 检查序列是否匹配
    if (RegistryHive->BaseBlock->Sequence1 !=
        RegistryHive->BaseBlock->Sequence2)
    {
        DPRINT1("The sequences DO NOT MATCH (Sequence1 == 0x%x, Sequence2 == 0x%x)\n",
                RegistryHive->BaseBlock->Sequence1, RegistryHive->BaseBlock->Sequence2);
        return FALSE;
    }

    // 计算位图和缓冲区大小
    BitmapSize = ROUND_UP(sizeof(ULONG) + RegistryHive->DirtyVector.SizeOfBitMap, HSECTOR_SIZE);
    BufferSize = HV_LOG_HEADER_SIZE + BitmapSize;

    // 分配基础头块缓冲区
    HeaderBuffer = static_cast<unsigned char*>(RegistryHive->Allocate(BufferSize, TRUE, TAG_CM));
    if (!HeaderBuffer)
    {
        DPRINT1("Couldn't allocate buffer for base header block\n");
        return FALSE;
    }

    // 清零缓冲区
    RtlZeroMemory(HeaderBuffer, BufferSize);

    // 更新蜂巢基础块并增加主序列号
    RegistryHive->BaseBlock->Type = HFILE_TYPE_LOG;
    RegistryHive->BaseBlock->Sequence1++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    // 复制基础块头
    RtlCopyMemory(HeaderBuffer, RegistryHive->BaseBlock, HV_LOG_HEADER_SIZE);
    Ptr = HeaderBuffer + HV_LOG_HEADER_SIZE;

    // 复制脏位图
    *((PULONG)Ptr) = HV_LOG_DIRTY_SIGNATURE;
    Ptr += sizeof(HV_LOG_DIRTY_SIGNATURE);

    // 标记脏块
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        LastIndex = BlockIndex;
        BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
        if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
        {
            break;
        }

        Ptr[BlockIndex] = HV_LOG_DIRTY_BLOCK;
        BlockIndex++;
    }

    // 写入蜂巢头和块位图到日志
    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                      &FileOffset, HeaderBuffer, BufferSize);
    RegistryHive->Free(HeaderBuffer, 0);
    if (!Success)
    {
        DPRINT1("Failed to write the hive header block to log (primary sequence)\n");
        return FALSE;
    }

    // 写入实际的脏数据到日志
    FileOffset = BufferSize;
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        LastIndex = BlockIndex;
        BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
        if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
        {
            break;
        }

        Block = reinterpret_cast<void*>(RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress);

        Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                          &FileOffset, Block, HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write dirty block to log (block 0x%p, block index 0x%x)\n", Block, BlockIndex);
            return FALSE;
        }

        BlockIndex++;
        FileOffset += HBLOCK_SIZE;
    }

    // 立即刷新日志
    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the log\n");
        return FALSE;
    }

    // 增加次序列号并再次刷新日志
    RegistryHive->BaseBlock->Sequence2++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      HV_LOG_HEADER_SIZE);
    if (!Success)
    {
        DPRINT1("Failed to write the log file (secondary sequence)\n");
        return FALSE;
    }

    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the log\n");
        return FALSE;
    }

    return TRUE;
}


/**
 * @brief 写入蜂巢数据
 *
 * 这个函数用于在同步操作期间将脏数据或非脏数据写入主蜂巢。蜂巢写入也可以在系统请求的刷新操作期间执行。
 *
 * @param RegistryHive 蜂巢指针
 * @param OnlyDirty 如果为TRUE，函数只查找脏数据写入主蜂巢，否则写入所有数据
 * @param FileType 注册表蜂巢的文件类型，可以是HFILE_TYPE_PRIMARY或HFILE_TYPE_ALTERNATE
 * @return 如果写入蜂巢成功，返回TRUE，否则返回FALSE
 */
static
BOOLEAN
CMAPI
HvpWriteHive(
    _In_ PHHIVE RegistryHive,
    _In_ BOOLEAN OnlyDirty,
    _In_ ULONG FileType)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LastIndex;
    void* Block;

    // 蜂巢必须是可写的且基础块长度有效
    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->BaseBlock->Length ==
           RegistryHive->Storage[Stable].Length * HBLOCK_SIZE);
    ASSERT(RegistryHive->BaseBlock->RootCell != HCELL_NIL);

    // 验证基础头
    HvpValidateBaseHeader(RegistryHive);

    // 检查序列是否匹配
    if (RegistryHive->BaseBlock->Sequence1 !=
        RegistryHive->BaseBlock->Sequence2)
    {
        DPRINT1("The sequences DO NOT MATCH (Sequence1 == 0x%x, Sequence2 == 0x%x)\n",
                RegistryHive->BaseBlock->Sequence1, RegistryHive->BaseBlock->Sequence2);
        return FALSE;
    }

    // 更新主序列号并写入基础块到蜂巢
    RegistryHive->BaseBlock->Type = HFILE_TYPE_PRIMARY;
    RegistryHive->BaseBlock->Sequence1++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, FileType,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      sizeof(HBASE_BLOCK));
    if (!Success)
    {
        DPRINT1("Failed to write the base block header to primary hive (primary sequence)\n");
        return FALSE;
    }

    // 写入整个主蜂巢，块 by 块
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        if (OnlyDirty)
        {
            LastIndex = BlockIndex;
            BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
            if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
            {
                break;
            }
        }

        Block = reinterpret_cast<void*>(RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress);
        FileOffset = (BlockIndex + 1) * HBLOCK_SIZE;

        Success = RegistryHive->FileWrite(RegistryHive, FileType,
                                          &FileOffset, Block, HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write hive block to primary hive file (block 0x%p, block index 0x%x)\n",
                    Block, BlockIndex);
            return FALSE;
        }

        BlockIndex++;
    }

    // 立即刷新蜂巢
    Success = RegistryHive->FileFlush(RegistryHive, FileType, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the primary hive\n");
        return FALSE;
    }

    // 增加次序列号并更新校验和
    RegistryHive->BaseBlock->Sequence2++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, FileType,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      sizeof(HBASE_BLOCK));
    if (!Success)
    {
        DPRINT1("Failed to write the base block header to primary hive (secondary sequence)\n");
        return FALSE;
    }

    Success = RegistryHive->FileFlush(RegistryHive, FileType, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the primary hive\n");
        return FALSE;
    }

    return TRUE;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief 同步蜂巢
 *
 * 这个函数用于将蜂巢与内存中的最新更新同步，包括脏数据。它将数据写入蜂巢日志和相应的主蜂巢。同步在系统请求的刷新操作期间执行。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果同步成功，返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvSyncHive(
    _In_ PHHIVE RegistryHive)
{
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    BOOLEAN HardErrors;
#endif

    // 蜂巢必须是可写的且签名有效
    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

    // 避免对易失蜂巢进行写操作
    if (RegistryHive->HiveFlags & HIVE_VOLATILE)
    {
        DPRINT("Hive 0x%p is volatile\n", RegistryHive);
        return TRUE;
    }

    // 检查脏位图中是否有脏数据
    if (RtlFindSetBits(&RegistryHive->DirtyVector, 1, 0) == ~HV_CLEAN_BLOCK)
    {
        DPRINT("The dirty vector has clean data, nothing to do\n");
        return TRUE;
    }

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    // 同步蜂巢前禁用硬错误
    // HardErrors = IoSetThreadHardErrorMode(FALSE);
#endif

#if !defined(_BLDR_)
    // 更新蜂巢头修改时间
    KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);
#endif

    // 如果有日志文件，更新日志文件
    if (RegistryHive->Log)
    {
        DPRINT1("Writing log file\n");
        if (!HvpWriteLog(RegistryHive))
        {
            DPRINT1("Failed to write a log whilst syncing the hive\n");
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
            // IoSetThreadHardErrorMode(HardErrors);
#endif
            return FALSE;
        }
        DPRINT1("Done writing log file\n");
    }

    // 更新主蜂巢文件
    if (!HvpWriteHive(RegistryHive, TRUE, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("Failed to write the primary hive\n");
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
        // IoSetThreadHardErrorMode(HardErrors);
#endif
        return FALSE;
    }

    // 如果有备用蜂巢文件，更新备用蜂巢文件
    if (RegistryHive->Alternate)
    {
        if (!HvpWriteHive(RegistryHive, TRUE, HFILE_TYPE_ALTERNATE))
        {
            DPRINT1("Failed to write the alternate hive\n");
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
            // IoSetThreadHardErrorMode(HardErrors);
#endif
            return FALSE;
        }
    }

    // 清除脏位图
    RtlClearAllBits(&RegistryHive->DirtyVector);
    RegistryHive->DirtyCount = 0;


    // IoSetThreadHardErrorMode(HardErrors);

    return TRUE;
}


/**
 * @brief 判断蜂巢是否需要收缩
 *
 * 这个函数用于判断蜂巢是否需要根据其整体大小进行收缩，以避免不必要的膨胀。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果蜂巢需要收缩，返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvHiveWillShrink(
    _In_ PHHIVE RegistryHive)
{
    // 尚未实现收缩功能
    UNIMPLEMENTED_ONCE;
    return FALSE;
}

/**
 * @brief 写入蜂巢数据
 *
 * 这个函数用于将整个注册表数据写入蜂巢，忽略数据块是否脏。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果写入蜂巢成功，返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvWriteHive(
    _In_ PHHIVE RegistryHive)
{
    // 蜂巢必须是可写的且签名有效
    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

#if !defined(_BLDR_)
    // 更新蜂巢头修改时间
    KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);
#endif

    // 更新蜂巢文件
    if (!HvpWriteHive(RegistryHive, FALSE, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("Failed to write the hive\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief 写入备用蜂巢数据
 *
 * 这个函数用于将数据写入备用蜂巢。备用蜂巢通常由主蜂巢备份。如果两个蜂巢不再匹配，此函数用于强制写入数据到备用蜂巢。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果写入备用蜂巢成功，返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvWriteAlternateHive(
    _In_ PHHIVE RegistryHive)
{
    // 蜂巢必须是可写的且签名有效
    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);
    ASSERT(RegistryHive->Alternate);

#if !defined(_BLDR_)
    // 更新蜂巢头修改时间
    KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);
#endif

    // 更新蜂巢文件
    if (!HvpWriteHive(RegistryHive, FALSE, HFILE_TYPE_ALTERNATE))
    {
        DPRINT1("Failed to write the alternate hive\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief 清理日志文件
 *
 * 在成功恢复后清理日志文件，将其大小重置为0。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果清理成功返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvpTruncateLog(
    _In_ PHHIVE RegistryHive)
{
    BOOLEAN Success;
    ULONG FileOffset = 0;
    HBASE_BLOCK EmptyHeader;

    if (!RegistryHive->Log)
        return TRUE;

    RtlZeroMemory(&EmptyHeader, sizeof(HBASE_BLOCK));
    EmptyHeader.Signature = HV_HBLOCK_SIGNATURE;
    EmptyHeader.Type = HFILE_TYPE_LOG;
    EmptyHeader.Format = HBASE_FORMAT_MEMORY;
    EmptyHeader.Sequence1 = RegistryHive->BaseBlock->Sequence1;
    EmptyHeader.Sequence2 = RegistryHive->BaseBlock->Sequence2;
    EmptyHeader.CheckSum = HvpHiveHeaderChecksum(&EmptyHeader);

    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                      &FileOffset, &EmptyHeader, sizeof(HBASE_BLOCK));
    if (Success)
    {
        Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
    }

    return Success;
}

/**
 * @brief 从恢复中同步蜂巢
 *
 * 这个函数用于在注册表的恢复/复活操作期间同步蜂巢与恢复的数据。
 *
 * @param RegistryHive 蜂巢指针
 * @return 如果同步恢复成功，返回TRUE，否则返回FALSE
 */
BOOLEAN
CMAPI
HvSyncHiveFromRecover(
    _In_ PHHIVE RegistryHive)
{
    BOOLEAN Success;

    ASSERT(!RegistryHive->ReadOnly);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

    Success = HvpWriteHive(RegistryHive, TRUE, HFILE_TYPE_PRIMARY);

    if (Success)
    {
        HvpTruncateLog(RegistryHive);
    }

    return Success;
}


/* EOF */
