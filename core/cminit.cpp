/*
 * PROJECT:   注册表操作库
 * LICENSE:   GPL - See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#define NDEBUG
#include "cmlib.hpp"

#include "debug.hpp"

ULONG CmlibTraceLevel = 0;

/**
 * @brief 创建根节点
 *
 * 这个函数用于创建一个蜂巢的根节点。
 *
 * @param Hive 蜂巢指针
 * @param Name 节点名称
 * @return 成功返回TRUE，失败返回FALSE
 */
BOOLEAN CMAPI
CmCreateRootNode(
    PHHIVE Hive,
    PCWSTR Name)
{
    UNICODE_STRING KeyName;
    PCM_KEY_NODE KeyCell;
    HCELL_INDEX RootCellIndex;

    // 初始化节点名称并分配内存
    RtlInitUnicodeString(&KeyName, Name);
    RootCellIndex = HvAllocateCell(Hive,
                                   FIELD_OFFSET(CM_KEY_NODE, Name) +
                                   CmpNameSize(Hive, &KeyName),
                                   Stable,
                                   HCELL_NIL);
    if (RootCellIndex == HCELL_NIL) return FALSE;

    // 设置基础块
    Hive->BaseBlock->RootCell = RootCellIndex;
    Hive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(Hive->BaseBlock);

    // 获取键单元格
    KeyCell = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, RootCellIndex));
    if (!KeyCell)
    {
        HvFreeCell(Hive, RootCellIndex);
        return FALSE;
    }

    // 设置单元格
    KeyCell->Signature = CM_KEY_NODE_SIGNATURE;
    KeyCell->Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;
    // KeQuerySystemTime(&KeyCell->LastWriteTime);
    KeyCell->LastWriteTime.QuadPart = 0ULL;
    KeyCell->Parent = HCELL_NIL;
    KeyCell->SubKeyCounts[Stable] = 0;
    KeyCell->SubKeyCounts[Volatile] = 0;
    KeyCell->SubKeyLists[Stable] = HCELL_NIL;
    KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
    KeyCell->ValueList.Count = 0;
    KeyCell->ValueList.List = HCELL_NIL;
    KeyCell->Security = HCELL_NIL;
    KeyCell->Class = HCELL_NIL;
    KeyCell->ClassLength = 0;
    KeyCell->MaxNameLen = 0;
    KeyCell->MaxClassLen = 0;
    KeyCell->MaxValueNameLen = 0;
    KeyCell->MaxValueDataLen = 0;
    KeyCell->NameLength = CmpCopyName(Hive, KeyCell->Name, &KeyName);
    if (KeyCell->NameLength < KeyName.Length) KeyCell->Flags |= KEY_COMP_NAME;

    // 返回成功
    HvReleaseCell(Hive, RootCellIndex);
    return TRUE;
}

