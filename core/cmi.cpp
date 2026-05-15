/*
 *  NTREG kernel
 *  Copyright (C) 2024 NTSoft
 *
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG hive maker
 * FILE:            cmi.c
 * PURPOSE:         Registry file manipulation routines
 * PROGRAMMERS:     Lily King
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include <stdio.h>
#include <stdlib.h>

#include "typedefs.hpp"

#include "cmlib.hpp"

#include "cmi.hpp"
#include "reg_internal.hpp"


/* FUNCTIONS ****************************************************************/

/**
 * @brief 分配内存。
 *
 * 该函数根据指定的内存大小和分页属性分配内存。
 *
 * @param Size 要分配的内存大小。
 * @param Paged 指示内存是否分页的布尔值。
 * @param Tag 内存分配的标签。
 *
 * @return 返回指向分配内存的指针，如果分配失败则返回 NULL。
 *
 * @note 该函数目前实现为使用 malloc 分配内存。
 */
void*
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    return malloc(static_cast<size_t>(Size));
}

/**
 * @brief 释放内存。
 *
 * 该函数释放先前分配的内存。
 *
 * @param Ptr 指向要释放的内存的指针。
 * @param Quota 内存配额。
 *
 * @note 该函数目前实现为使用 free 释放内存。
 */
void
CmpFree(
    IN void* Ptr,
    IN ULONG Quota)
{
    free(Ptr);
}

/**
 * @brief 从注册表文件读取数据。
 *
 * 该函数从指定的注册表文件中读取数据到缓冲区。
 *
 * @param RegistryHive 指向注册表蜂巢结构的指针。
 * @param FileType 文件类型。
 * @param FileOffset 指向文件偏移量的指针。
 * @param Buffer 指向存储读取数据的缓冲区的指针。
 * @param BufferLength 缓冲区的长度。
 *
 * @return 如果成功读取数据，返回 TRUE；否则返回 FALSE。
 *
 * @note 该函数目前实现为使用 fseek 和 fread 从文件中读取数据。
 */
BOOLEAN
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    OUT void* Buffer,
    IN SIZE_T BufferLength)
{
    DPRINT1("CmpFileRead called\n");
    PCMHIVE CmHive = reinterpret_cast<PCMHIVE>(RegistryHive);
    FILE *File = static_cast<FILE*>(CmHive->FileHandles[HFILE_TYPE_PRIMARY]);
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
        return FALSE;

    return (fread(Buffer, 1, BufferLength, File) == BufferLength);
}


/**
 * @brief 写入注册表文件。
 *
 * 该函数将数据写入指定的注册表文件。
 *
 * @param RegistryHive 指向注册表蜂巢结构的指针。
 * @param FileType 文件类型。
 * @param FileOffset 指向文件偏移量的指针。
 * @param Buffer 指向要写入数据的缓冲区的指针。
 * @param BufferLength 缓冲区的长度。
 *
 * @return 如果成功写入数据，返回 TRUE；否则返回 FALSE。
 *
 * @note 该函数目前实现为使用 fseek 和 fwrite 将数据写入文件。
 */
BOOLEAN
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    IN void* Buffer,
    IN SIZE_T BufferLength)
{
    DPRINT1("CmpFileWrite called\n");
    PCMHIVE CmHive = reinterpret_cast<PCMHIVE>(RegistryHive);
    FILE *File = static_cast<FILE*>(CmHive->FileHandles[HFILE_TYPE_PRIMARY]);
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
        return FALSE;

    return (fwrite(Buffer, 1, BufferLength, File) == BufferLength);
}

/**
 * @brief 设置注册表文件的大小。
 *
 * 该函数设置指定的注册表文件的大小。
 *
 * @param RegistryHive 指向注册表蜂巢结构的指针。
 * @param FileType 文件类型。
 * @param FileSize 新的文件大小。
 * @param OldFileSize 旧的文件大小。
 *
 * @return 如果成功设置文件大小，返回 TRUE；否则返回 FALSE。
 *
 * @note 该函数目前实现为未实现，总是返回 FALSE。
 */
BOOLEAN
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize)
{
    DPRINT1("CmpFileSetSize() unimplemented\n");
    return FALSE;
}

/**
 * @brief 刷新注册表文件。
 *
 * 该函数刷新指定的注册表文件，确保所有数据都被写入磁盘。
 *
 * @param RegistryHive 指向注册表蜂巢结构的指针。
 * @param FileType 文件类型。
 * @param FileOffset 指向文件偏移量的指针。
 * @param Length 要刷新的数据长度。
 *
 * @return 如果成功刷新文件，返回 TRUE；否则返回 FALSE。
 *
 * @note 该函数目前实现为使用 fflush 刷新文件。
 */
BOOLEAN
CmpFileFlush(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length)
{
    PCMHIVE CmHive = reinterpret_cast<PCMHIVE>(RegistryHive);
    FILE *File = static_cast<FILE*>(CmHive->FileHandles[HFILE_TYPE_PRIMARY]);
    return (fflush(File) == 0);
}


/**
 * @brief 初始化注册表蜂巢。
 *
 * 该函数初始化一个注册表蜂巢结构，并将其添加到蜂巢列表中。
 *
 * @param Hive 指向要初始化的注册表蜂巢结构的指针。
 * @param Name 指向蜂巢名称的指针。
 *
 * @return 如果初始化成功，返回 STATUS_SUCCESS；否则返回相应的错误状态。
 *
 * @note 该函数目前实现为初始化蜂巢结构并调用 HvInitialize 进行进一步初始化。
 */
int
CmiInitializeHive(
    IN OUT PCMHIVE Hive,
    IN PCWSTR Name)
{
    int Status;

    RtlZeroMemory(Hive, sizeof(*Hive));

    DPRINT("Hive 0x%p\n", Hive);

    Status = HvInitialize(&Hive->Hive,
                          HINIT_CREATE,
                          HIVE_NOLAZYFLUSH,
                          HFILE_TYPE_PRIMARY,
                          0,
                          CmpAllocate,
                          CmpFree,
                          CmpFileSetSize,
                          CmpFileWrite,
                          CmpFileRead,
                          CmpFileFlush,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (!CmCreateRootNode(&Hive->Hive, Name))
    {
        HvFree(&Hive->Hive);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Add the new hive to the hive list */
    InsertTailList(&CmiHiveListHead,
                   &Hive->HiveList);

    return STATUS_SUCCESS;
}

/**
 * @brief 创建安全密钥。
 *
 * 该函数为指定的注册表蜂巢创建一个安全密钥。
 *
 * @param Hive 指向注册表蜂巢结构的指针。
 * @param Cell 蜂巢单元索引。
 * @param Descriptor 指向安全描述符的指针。
 * @param DescriptorLength 安全描述符的长度。
 *
 * @return 如果创建成功，返回 STATUS_SUCCESS；否则返回相应的错误状态。
 *
 * @note 该函数目前实现为分配安全单元并设置相应的安全描述符。
 */
int
CmiCreateSecurityKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN unsigned char* Descriptor,
    IN ULONG DescriptorLength)
{
    HCELL_INDEX SecurityCell;
    PCM_KEY_NODE Node;
    PCM_KEY_SECURITY Security;

    Node = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Cell));
    SecurityCell = HvAllocateCell(Hive,
                                  FIELD_OFFSET(CM_KEY_SECURITY, Descriptor) +
                                  DescriptorLength,
                                  Stable,
                                  HCELL_NIL);
    if (SecurityCell == HCELL_NIL)
    {
        HvReleaseCell(Hive, Cell);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Node->Security = SecurityCell;
    Security = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(Hive, SecurityCell));
    Security->Signature = CM_KEY_SECURITY_SIGNATURE;
    Security->ReferenceCount = 1;
    Security->DescriptorLength = DescriptorLength;

    RtlMoveMemory(&Security->Descriptor,
                  Descriptor,
                  DescriptorLength);

    Security->Flink = Security->Blink = SecurityCell;

    HvReleaseCell(Hive, SecurityCell);
    HvReleaseCell(Hive, Cell);

    return STATUS_SUCCESS;
}


/**
 * @brief 创建子键
 *
 * 该函数用于在注册表中创建一个新的子键。
 *
 * @param RegistryHive 注册表蜂巢结构的指针
 * @param ParentKeyCellOffset 父键的单元格偏移量
 * @param SubKeyName 子键的名称
 * @param VolatileKey 是否为易失性键
 * @param pNKBOffset 输出参数，新键的单元格偏移量
 * @return int 状态码，成功返回STATUS_SUCCESS
 */
static int
CmiCreateSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN BOOLEAN VolatileKey,
    OUT HCELL_INDEX* pNKBOffset)
{
    HCELL_INDEX NKBOffset;
    PCM_KEY_NODE NewKeyCell;
    UNICODE_STRING KeyName;
    HSTORAGE_TYPE Storage;

    /* 如果子键名称以路径分隔符开头，则跳过该分隔符 */
    if (SubKeyName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        KeyName.Buffer = &SubKeyName->Buffer[1];
        KeyName.Length = KeyName.MaximumLength = SubKeyName->Length - sizeof(WCHAR);
    }
    else
    {
        KeyName = *SubKeyName;
    }

    /* 确定存储类型（易失性或稳定） */
    Storage = (VolatileKey ? Volatile : Stable);

    /* 分配新的键节点单元格 */
    NKBOffset = HvAllocateCell(&RegistryHive->Hive,
                               FIELD_OFFSET(CM_KEY_NODE, Name) +
                               CmpNameSize(&RegistryHive->Hive, &KeyName),
                               Storage,
                               HCELL_NIL);
    if (NKBOffset == HCELL_NIL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* 获取新键节点的指针 */
    NewKeyCell = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(&RegistryHive->Hive, NKBOffset));
    if (NewKeyCell == NULL)
    {
        HvFreeCell(&RegistryHive->Hive, NKBOffset);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* 初始化新键节点 */
    NewKeyCell->Signature = CM_KEY_NODE_SIGNATURE;
    NewKeyCell->Flags = (VolatileKey ? KEY_IS_VOLATILE : 0);
    KeQuerySystemTime(&NewKeyCell->LastWriteTime);
    NewKeyCell->Parent = ParentKeyCellOffset;
    NewKeyCell->SubKeyCounts[Stable] = 0;
    NewKeyCell->SubKeyCounts[Volatile] = 0;
    NewKeyCell->SubKeyLists[Stable] = HCELL_NIL;
    NewKeyCell->SubKeyLists[Volatile] = HCELL_NIL;
    NewKeyCell->ValueList.Count = 0;
    NewKeyCell->ValueList.List = HCELL_NIL;
    NewKeyCell->Security = HCELL_NIL;
    NewKeyCell->Class = HCELL_NIL;
    NewKeyCell->ClassLength = 0;
    NewKeyCell->MaxNameLen = 0;
    NewKeyCell->MaxClassLen = 0;
    NewKeyCell->MaxValueNameLen = 0;
    NewKeyCell->MaxValueDataLen = 0;
    NewKeyCell->NameLength = CmpCopyName(&RegistryHive->Hive, NewKeyCell->Name, &KeyName);
    if (NewKeyCell->NameLength < KeyName.Length) NewKeyCell->Flags |= KEY_COMP_NAME;

    /* 继承父键的安全属性 */
    if (ParentKeyCellOffset == HCELL_NIL)
    {
        // 实际上是在创建根键
        ASSERT(FALSE);
    }
    else
    {
        /* 获取父键节点 */
        PCM_KEY_NODE ParentKeyCell;
        ParentKeyCell = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(&RegistryHive->Hive, ParentKeyCellOffset));

        if (ParentKeyCell)
        {
            /* 继承父键的安全块 */
            NewKeyCell->Security = ParentKeyCell->Security;
            if (NewKeyCell->Security != HCELL_NIL)
            {
                PCM_KEY_SECURITY Security;
                Security = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(&RegistryHive->Hive, NewKeyCell->Security));
                ++Security->ReferenceCount;
                HvReleaseCell(&RegistryHive->Hive, NewKeyCell->Security);
            }

            HvReleaseCell(&RegistryHive->Hive, ParentKeyCellOffset);
        }
    }

    HvReleaseCell(&RegistryHive->Hive, NKBOffset);

    *pNKBOffset = NKBOffset;
    return STATUS_SUCCESS;
}


/**
 * @brief 添加子键
 *
 * 该函数用于在注册表中添加一个新的子键。
 *
 * @param RegistryHive 注册表蜂巢结构的指针
 * @param ParentKeyCellOffset 父键的单元格偏移量
 * @param SubKeyName 子键的名称
 * @param VolatileKey 是否为易失性键
 * @param pBlockOffset 输出参数，新键的单元格偏移量
 * @return int 状态码，成功返回STATUS_SUCCESS
 */
int
CmiAddSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN BOOLEAN VolatileKey,
    OUT HCELL_INDEX *pBlockOffset)
{
    PCM_KEY_NODE ParentKeyCell;
    HCELL_INDEX NKBOffset;
    int Status;

    /* 创建新的子键 */
    Status = CmiCreateSubKey(RegistryHive, ParentKeyCellOffset, SubKeyName, VolatileKey, &NKBOffset);
    if (!NT_SUCCESS(Status))
        return Status;

    /* 标记父键单元格为脏 */
    HvMarkCellDirty(&RegistryHive->Hive, ParentKeyCellOffset, FALSE);

    /* 将新键添加到父键的子键列表中 */
    if (!CmpAddSubKey(&RegistryHive->Hive, ParentKeyCellOffset, NKBOffset))
    {
        HvFreeCell(&RegistryHive->Hive, NKBOffset);
        return STATUS_UNSUCCESSFUL;
    }

    /* 获取父键节点 */
    ParentKeyCell = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(&RegistryHive->Hive, ParentKeyCellOffset));
    if (!ParentKeyCell)
    {
        CmpRemoveSubKey(&RegistryHive->Hive, ParentKeyCellOffset, NKBOffset);
        HvFreeCell(&RegistryHive->Hive, NKBOffset);
        return STATUS_UNSUCCESSFUL;
    }
    VERIFY_KEY_CELL(ParentKeyCell);

    /* 更新时间戳 */
    KeQuerySystemTime(&ParentKeyCell->LastWriteTime);

    /* 检查是否需要更新最大名称长度，如果需要则更新 */
    if (ParentKeyCell->MaxNameLen < SubKeyName->Length)
        ParentKeyCell->MaxNameLen = SubKeyName->Length;

    /* 释放单元格 */
    HvReleaseCell(&RegistryHive->Hive, ParentKeyCellOffset);

    *pBlockOffset = NKBOffset;
    return STATUS_SUCCESS;
}

/**
 * @brief 添加值键
 *
 * 该函数用于在注册表中添加一个新的值键。
 *
 * @param RegistryHive 注册表蜂巢结构的指针
 * @param Parent 父键节点
 * @param ChildIndex 子键索引
 * @param ValueName 值键的名称
 * @param pValueCell 输出参数，新值键的单元格指针
 * @param pValueCellOffset 输出参数，新值键的单元格偏移量
 * @return int 状态码，成功返回STATUS_SUCCESS
 */
int
CmiAddValueKey(
    IN PCMHIVE RegistryHive,
    IN PCM_KEY_NODE Parent,
    IN ULONG ChildIndex,
    IN PCUNICODE_STRING ValueName,
    OUT PCM_KEY_VALUE *pValueCell,
    OUT HCELL_INDEX *pValueCellOffset)
{
    int Status;
    HSTORAGE_TYPE Storage;
    PCM_KEY_VALUE NewValueCell;
    HCELL_INDEX NewValueCellOffset;

    /* 确定存储类型（易失性或稳定） */
    Storage = (Parent->Flags & KEY_IS_VOLATILE) ? Volatile : Stable;

    /* 分配新的值键单元格 */
    NewValueCellOffset = HvAllocateCell(&RegistryHive->Hive,
                               FIELD_OFFSET(CM_KEY_VALUE, Name) +
                               CmpNameSize(&RegistryHive->Hive, const_cast<PUNICODE_STRING>(ValueName)),
                               Storage,
                               HCELL_NIL);
    if (NewValueCellOffset == HCELL_NIL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* 获取新值键的指针 */
    NewValueCell = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(&RegistryHive->Hive, NewValueCellOffset));
    if (NewValueCell == NULL)
    {
        HvFreeCell(&RegistryHive->Hive, NewValueCellOffset);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* 初始化新值键 */
    NewValueCell->Signature = CM_KEY_VALUE_SIGNATURE;
    NewValueCell->NameLength = CmpCopyName(&RegistryHive->Hive,
                                           NewValueCell->Name,
                                           const_cast<PUNICODE_STRING>(ValueName));

    /* 检查是否为压缩名称 */
    if (NewValueCell->NameLength < ValueName->Length)
    {
        /* 这是一个压缩名称 */
        NewValueCell->Flags = VALUE_COMP_NAME;
    }
    else
    {
        /* 没有标志需要设置 */
        NewValueCell->Flags = 0;
    }

    NewValueCell->Type = 0;
    NewValueCell->DataLength = 0;
    NewValueCell->Data = HCELL_NIL;

    /* 标记单元格为脏 */
    HvMarkCellDirty(&RegistryHive->Hive, NewValueCellOffset, FALSE);

    /* 检查是否已经有值列表，如果有则确保其有效并标记为脏 */
    if (Parent->ValueList.Count)
    {
        ASSERT(Parent->ValueList.List != HCELL_NIL);
        HvMarkCellDirty(&RegistryHive->Hive, Parent->ValueList.List, FALSE);
    }

    /* 将新值键添加到子键列表中 */
    Status = CmpAddValueToList(&RegistryHive->Hive,
                               NewValueCellOffset,
                               ChildIndex,
                               Storage,
                               &Parent->ValueList);

    /* 如果添加失败，释放整个单元格，包括数据 */
    if (!NT_SUCCESS(Status))
    {
        CmpFreeValue(&RegistryHive->Hive, NewValueCellOffset);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        *pValueCell = NewValueCell;
        *pValueCellOffset = NewValueCellOffset;
        Status = STATUS_SUCCESS;
    }

    return Status;
}
