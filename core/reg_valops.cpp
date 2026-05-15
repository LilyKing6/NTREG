/*
 * PROJECT:   Registry Library
 * FILE:      reg_valops.cpp
 * PURPOSE:   Registry value operations (query, set, delete, enum)
 */

#define NDEBUG

#include "cmlib.hpp"
#include "reg_internal.hpp"

void
RepGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE ValueCell,
    OUT PULONG Type OPTIONAL,
    OUT unsigned char* Data OPTIONAL,
    IN OUT PULONG DataSize OPTIONAL)
{
    ULONG DataLength;
    void* DataCell;

    if (Type != NULL)
        *Type = ValueCell->Type;

    if (DataSize != NULL)
    {
        DataCell = CmpValueToData(Hive, ValueCell, &DataLength);

        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          DataCell,
                          mininum(*DataSize, DataLength));
        }

        *DataSize = DataLength;
    }
}

LONG
NTRegEnumValue(
    IN REGHKEY Key,
    IN ULONG Index,
    OUT PWCHAR ValueName,
    IN OUT PULONG NameSize,
    OUT PULONG Type OPTIONAL,
    OUT unsigned char* Data OPTIONAL,
    IN OUT PULONG DataSize OPTIONAL)
{
    PMEMKEY ParentKey = REGHKEY_TO_MEMKEY(Key);
    PHHIVE Hive = &ParentKey->RegistryHive->Hive;

    PCM_KEY_NODE KeyNode;
    PCELL_DATA ValueListCell;
    PCM_KEY_VALUE ValueCell;
    USHORT NameLength;

    DPRINT("NTRegEnumValue(%p, %lu, %S, %p, %p, %p, %p (%lu))\n",
          Key, Index, ValueName, NameSize, Type, Data, DataSize, *DataSize);

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, ParentKey->KeyCellOffset));
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if ((KeyNode->ValueList.Count == 0) ||
        (KeyNode->ValueList.List == HCELL_NIL) ||
        (Index >= KeyNode->ValueList.Count))
    {
        DPRINT("NTRegEnumValue: index invalid\n");
        return ERROR_NO_MORE_ITEMS;
    }

    ValueListCell = reinterpret_cast<PCELL_DATA>(HvGetCell(Hive, KeyNode->ValueList.List));
    ASSERT(ValueListCell != NULL);

    ValueCell = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(Hive, ValueListCell->u.KeyList[Index]));
    ASSERT(ValueCell != NULL);
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);

    if (ValueCell->Flags & VALUE_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(ValueCell->Name, ValueCell->NameLength);
        CmpCopyCompressedName(ValueName,
                              *NameSize,
                              ValueCell->Name,
                              ValueCell->NameLength);
    }
    else
    {
        NameLength = ValueCell->NameLength;
        RtlCopyMemory(ValueName, ValueCell->Name,
                      mininum(*NameSize, ValueCell->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        ValueName[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, ValueListCell->u.KeyList[Index]);
    HvReleaseCell(Hive, KeyNode->ValueList.List);
    HvReleaseCell(Hive, REGHKEY_TO_HCI(Key));

    DPRINT("NTRegEnumValue done -> %u, '%.*S'\n", *NameSize, *NameSize, ValueName);
    return ERROR_SUCCESS;
}

LONG
NTRegSetValueExW(
    IN REGHKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL,
    IN ULONG Reserved,
    IN ULONG dwType,
    IN const unsigned char* lpData,
    IN ULONG cbData)
{
    PMEMKEY Key = REGHKEY_TO_MEMKEY(hKey);
    PHHIVE Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    ULONG ChildIndex;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    void* DataCell;
    ULONG DataCellSize;
    int Status;

    if (dwType == REG_LINK)
    {
        PMEMKEY DestKey;

        if (cbData != sizeof(void*))
            return ERROR_INVALID_PARAMETER;

        DestKey = REGHKEY_TO_MEMKEY(*reinterpret_cast<PREGHKEY>(const_cast<unsigned char*>(lpData)));

        if (!DestKey || !DestKey->RegistryHive || DestKey->KeyCellOffset == HCELL_NIL)
            return ERROR_INVALID_PARAMETER;

        if (Key->RegistryHive != DestKey->RegistryHive)
            return ERROR_SUCCESS;

        DPRINT1("Save link to registry\n");
        return ERROR_INVALID_FUNCTION;
    }

    if ((cbData & ~CM_KEY_VALUE_SPECIAL_SIZE) != cbData)
        return ERROR_GEN_FAILURE;

    Hive = &Key->RegistryHive->Hive;

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Key->KeyCellOffset));
    if (!KeyNode)
        return ERROR_GEN_FAILURE;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    HvMarkCellDirty(Hive, Key->KeyCellOffset, FALSE);

    RtlInitUnicodeString(&ValueNameString, lpValueName);
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           &ValueNameString,
                           &ChildIndex,
                           &CellIndex))
    {
        ASSERT(CellIndex == HCELL_NIL);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (CellIndex == HCELL_NIL)
    {
        Status = CmiAddValueKey(Key->RegistryHive,
                                KeyNode,
                                ChildIndex,
                                &ValueNameString,
                                &ValueCell,
                                &CellIndex);
    }
    else
    {
        ValueCell = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(&Key->RegistryHive->Hive, CellIndex));
        ASSERT(ValueCell != NULL);
        Status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(Status))
        return ERROR_GEN_FAILURE;

    if (!(ValueCell->DataLength & CM_KEY_VALUE_SPECIAL_SIZE) &&
         (ValueCell->DataLength & ~CM_KEY_VALUE_SPECIAL_SIZE) != 0)
    {
        DataCell = HvGetCell(Hive, ValueCell->Data);
        if (!DataCell)
            return ERROR_GEN_FAILURE;

        DataCellSize = (ULONG)(-HvGetCellSize(Hive, DataCell));
    }
    else
    {
        DataCell = NULL;
        DataCellSize = 0;
    }

    if (cbData <= sizeof(HCELL_INDEX))
    {
        DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);
        if (DataCell)
            HvFreeCell(Hive, ValueCell->Data);

        RtlCopyMemory(&ValueCell->Data, lpData, cbData);
        ValueCell->DataLength = (cbData | CM_KEY_VALUE_SPECIAL_SIZE);
        ValueCell->Type = dwType;
    }
    else
    {
        if (cbData > DataCellSize)
        {
            HCELL_INDEX NewOffset;

            DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);

            NewOffset = HvAllocateCell(Hive, cbData, Stable, HCELL_NIL);
            if (NewOffset == HCELL_NIL)
            {
                DPRINT("HvAllocateCell() has failed!\n");
                return ERROR_GEN_FAILURE;
            }

            if (DataCell)
                HvFreeCell(Hive, ValueCell->Data);

            ValueCell->Data = NewOffset;
            DataCell = static_cast<void*>(HvGetCell(Hive, NewOffset));
        }

        RtlCopyMemory(DataCell, lpData, cbData);
        ValueCell->DataLength = (cbData & ~CM_KEY_VALUE_SPECIAL_SIZE);
        ValueCell->Type = dwType;
        HvMarkCellDirty(Hive, ValueCell->Data, FALSE);
    }

    HvMarkCellDirty(Hive, CellIndex, FALSE);

    if (KeyNode->MaxValueNameLen < ValueNameString.Length)
        KeyNode->MaxValueNameLen = ValueNameString.Length;

    if (KeyNode->MaxValueDataLen < cbData)
        KeyNode->MaxValueDataLen = cbData;

    KeQuerySystemTime(&KeyNode->LastWriteTime);

    HvReleaseCell(Hive, Key->KeyCellOffset);

    return ERROR_SUCCESS;
}

LONG
NTRegQueryValueExW(
    IN REGHKEY hKey,
    IN LPCWSTR lpValueName,
    IN PULONG lpReserved,
    OUT PULONG lpType OPTIONAL,
    OUT unsigned char* lpData OPTIONAL,
    IN OUT PULONG lpcbData OPTIONAL)
{
    PMEMKEY ParentKey = REGHKEY_TO_MEMKEY(hKey);
    PHHIVE Hive = &ParentKey->RegistryHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, ParentKey->KeyCellOffset));
    if (!KeyNode)
        return ERROR_GEN_FAILURE;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    RtlInitUnicodeString(&ValueNameString, lpValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
        return ERROR_FILE_NOT_FOUND;

    ValueCell = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(Hive, CellIndex));
    ASSERT(ValueCell != NULL);

    RepGetValueData(Hive, ValueCell, lpType, lpData, lpcbData);

    HvReleaseCell(Hive, CellIndex);

    return ERROR_SUCCESS;
}

LONG
NTRegDeleteValueW(
    IN REGHKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL)
{
    LONG rc;
    int Status;
    PMEMKEY Key = REGHKEY_TO_MEMKEY(hKey);
    PHHIVE Hive = &Key->RegistryHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    ULONG ChildIndex;
    UNICODE_STRING ValueNameString;

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Key->KeyCellOffset));
    if (!KeyNode)
        return ERROR_GEN_FAILURE;

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    RtlInitUnicodeString(&ValueNameString, lpValueName);
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           &ValueNameString,
                           &ChildIndex,
                           &CellIndex))
    {
        ASSERT(CellIndex == HCELL_NIL);
    }
    if (CellIndex == HCELL_NIL)
    {
        rc = ERROR_FILE_NOT_FOUND;
        goto Quit;
    }

    HvMarkCellDirty(Hive, Key->KeyCellOffset, FALSE);
    HvMarkCellDirty(Hive, KeyNode->ValueList.List, FALSE);
    HvMarkCellDirty(Hive, CellIndex, FALSE);

    ValueCell = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(Hive, CellIndex));
    ASSERT(ValueCell);

    if (!CmpMarkValueDataDirty(Hive, ValueCell))
    {
        rc = ERROR_NO_LOG_SPACE;
        goto Quit;
    }

    ASSERT(HvIsCellDirty(Hive, KeyNode->ValueList.List));
    ASSERT(HvIsCellDirty(Hive, CellIndex));

    Status = CmpRemoveValueFromList(Hive, ChildIndex, &KeyNode->ValueList);
    if (!NT_SUCCESS(Status))
    {
        rc = ERROR_NO_SYSTEM_RESOURCES;
        goto Quit;
    }

    if (!CmpFreeValue(Hive, CellIndex))
    {
        rc = ERROR_NO_SYSTEM_RESOURCES;
        goto Quit;
    }

    KeQuerySystemTime(&KeyNode->LastWriteTime);

    ASSERT(HvIsCellDirty(Hive, Key->KeyCellOffset));

    if (!KeyNode->ValueList.Count)
    {
        KeyNode->MaxValueNameLen = 0;
        KeyNode->MaxValueDataLen = 0;
    }

    rc = ERROR_SUCCESS;

Quit:
    if (ValueCell)
    {
        ASSERT(CellIndex != HCELL_NIL);
        HvReleaseCell(Hive, CellIndex);
    }

    if (KeyNode)
        HvReleaseCell(Hive, Key->KeyCellOffset);

    return rc;
}
