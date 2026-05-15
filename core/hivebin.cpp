/*
 * PROJECT:   注册表操作库
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#include "cmlib.hpp"

/**
 * @brief 添加一个新的Bin到Hive中。
 *
 * 该函数在Hive中添加一个新的Bin，并初始化其相关数据结构。
 *
 * @param RegistryHive 注册表Hive结构体指针。
 * @param Size 新Bin的大小（以字节为单位）。
 * @param Storage 存储类型。
 * @return 新添加的Bin结构体指针，如果失败则返回NULL。
 */
PHBIN CMAPI
HvpAddBin(
    PHHIVE RegistryHive,
    ULONG Size,
    HSTORAGE_TYPE Storage)
{
    PHMAP_ENTRY BlockList;
    PHBIN Bin;
    ULONG BinSize;
    ULONG i;
    ULONG BitmapSize;
    ULONG BlockCount;
    ULONG OldBlockListSize;
    PHCELL Block;

    /* 计算Bin的大小，向上取整到HBLOCK_SIZE的倍数 */
    BinSize = ROUND_UP(Size + sizeof(HBIN), HBLOCK_SIZE);
    BlockCount = BinSize / HBLOCK_SIZE;

    /* 分配新的Bin */
    Bin = static_cast<PHBIN>(RegistryHive->Allocate(BinSize, TRUE, TAG_CM));
    if (Bin == NULL)
        return NULL;
    RtlZeroMemory(Bin, BinSize);

    /* 初始化Bin的签名和文件偏移量 */
    Bin->Signature = HV_HBIN_SIGNATURE;
    Bin->FileOffset = RegistryHive->Storage[Storage].Length *
                      HBLOCK_SIZE;
    Bin->Size = BinSize;

    /* 分配新的块列表 */
    OldBlockListSize = RegistryHive->Storage[Storage].Length;
    BlockList = static_cast<PHMAP_ENTRY>(RegistryHive->Allocate(sizeof(HMAP_ENTRY) *
                                       (OldBlockListSize + BlockCount),
                                       TRUE,
                                       TAG_CM));
    if (BlockList == NULL)
    {
        RegistryHive->Free(Bin, 0);
        return NULL;
    }

    /* 复制旧的块列表到新的块列表 */
    if (OldBlockListSize > 0)
    {
        RtlCopyMemory(BlockList, RegistryHive->Storage[Storage].BlockList,
                      OldBlockListSize * sizeof(HMAP_ENTRY));
        RegistryHive->Free(RegistryHive->Storage[Storage].BlockList, 0);
    }

    RegistryHive->Storage[Storage].BlockList = BlockList;
    RegistryHive->Storage[Storage].Length += BlockCount;

    /* 初始化新的块列表 */
    for (i = 0; i < BlockCount; i++)
    {
        RegistryHive->Storage[Storage].BlockList[OldBlockListSize + i].BlockAddress =
            (reinterpret_cast<ULONG_PTR>(Bin) + (i * HBLOCK_SIZE));
        RegistryHive->Storage[Storage].BlockList[OldBlockListSize + i].BinAddress = reinterpret_cast<ULONG_PTR>(Bin);
    }

    /* 初始化这个堆中的一个空闲块 */
    Block = (PHCELL)(Bin + 1);
    Block->Size = (LONG)(BinSize - sizeof(HBIN));

    if (Storage == Stable)
    {
        /* 计算位图大小（总是32位的倍数） */
        BitmapSize = ROUND_UP(RegistryHive->Storage[Stable].Length,
                              sizeof(ULONG) * 8) / 8;

        /* 如果需要，扩展位图 */
        if (BitmapSize > RegistryHive->DirtyVector.SizeOfBitMap / 8)
        {
            PULONG BitmapBuffer;

            BitmapBuffer = static_cast<PULONG>(RegistryHive->Allocate(BitmapSize, TRUE, TAG_CM));
            RtlZeroMemory(BitmapBuffer, BitmapSize);
            if (RegistryHive->DirtyVector.SizeOfBitMap > 0)
            {
                ASSERT(RegistryHive->DirtyVector.Buffer);
                RtlCopyMemory(BitmapBuffer,
                              RegistryHive->DirtyVector.Buffer,
                              RegistryHive->DirtyVector.SizeOfBitMap / 8);
                RegistryHive->Free(RegistryHive->DirtyVector.Buffer, 0);
            }
            RtlInitializeBitMap(&RegistryHive->DirtyVector, BitmapBuffer,
                                BitmapSize * 8);
        }

        /* 标记新的Bin为脏 */
        RtlSetBits(&RegistryHive->DirtyVector,
                   Bin->FileOffset / HBLOCK_SIZE,
                   BlockCount);

        /* 更新基础块中的大小 */
        RegistryHive->BaseBlock->Length += BinSize;
    }

    return Bin;
}

