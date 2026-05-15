/*
 * PROJECT:   Registry Library
 * FILE:      reg_path.cpp
 * PURPOSE:   Registry path parsing and key traversal
 */

#define NDEBUG

#include "cmlib.hpp"
#include "reg_internal.hpp"

int
RegpCreateOrOpenKey(
    IN REGHKEY hParentKey,
    IN PCWSTR KeyName,
    IN BOOL AllowCreation,
    IN BOOL Volatile,
    OUT PREGHKEY Key)
{
    int Status;
    PWSTR LocalKeyName;
    PWSTR End;
    UNICODE_STRING KeyString;
    PREPARSE_POINT CurrentReparsePoint;
    PMEMKEY CurrentKey;
    PCMHIVE ParentRegistryHive;
    HCELL_INDEX ParentCellOffset;
    PCM_KEY_NODE ParentKeyCell;
    PLIST_ENTRY Ptr;
    HCELL_INDEX BlockOffset;

    DPRINT("RegpCreateOrOpenKey('%S')\n", KeyName);

    if (*KeyName == OBJ_NAME_PATH_SEPARATOR)
    {
        KeyName++;
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else if (hParentKey == NULL)
    {
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else
    {
        ParentRegistryHive = REGHKEY_TO_MEMKEY(hParentKey)->RegistryHive;
        ParentCellOffset = REGHKEY_TO_MEMKEY(hParentKey)->KeyCellOffset;
    }

    LocalKeyName = const_cast<PWSTR>(KeyName);
    for (;;)
    {
        End = const_cast<PWSTR>(strchrW(LocalKeyName, OBJ_NAME_PATH_SEPARATOR));
        if (End)
        {
            KeyString.Buffer = LocalKeyName;
            KeyString.Length = KeyString.MaximumLength =
                (USHORT)((ULONG_PTR)End - (ULONG_PTR)LocalKeyName);
        }
        else
        {
            RtlInitUnicodeString(&KeyString, LocalKeyName);
            if (KeyString.Length == 0)
            {
                break;
            }
        }

        ParentKeyCell = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(&ParentRegistryHive->Hive, ParentCellOffset));
        if (!ParentKeyCell)
            return ERROR_GEN_FAILURE;

        VERIFY_KEY_CELL(ParentKeyCell);

        BlockOffset = CmpFindSubKeyByName(&ParentRegistryHive->Hive, ParentKeyCell, &KeyString);
        if (BlockOffset != HCELL_NIL)
        {
            Status = STATUS_SUCCESS;

            Ptr = CmiReparsePointsHead.Flink;
            while (Ptr != &CmiReparsePointsHead)
            {
                CurrentReparsePoint = CONTAINING_RECORD(Ptr, REPARSE_POINT, ListEntry);
                if (CurrentReparsePoint->SourceHive == ParentRegistryHive &&
                    CurrentReparsePoint->SourceKeyCellOffset == BlockOffset)
                {
                    ParentRegistryHive = CurrentReparsePoint->DestinationHive;
                    BlockOffset = CurrentReparsePoint->DestinationKeyCellOffset;
                    break;
                }
                Ptr = Ptr->Flink;
            }
        }
        else if (AllowCreation)
        {
            Status = CmiAddSubKey(ParentRegistryHive,
                                  ParentCellOffset,
                                  &KeyString,
                                  Volatile,
                                  &BlockOffset);
        }
        else
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        HvReleaseCell(&ParentRegistryHive->Hive, ParentCellOffset);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("RegpCreateOrOpenKey('%S'): Could not create or open subkey '%.*S', Status 0x%08x\n",
                   KeyName, (int)(KeyString.Length / sizeof(WCHAR)), KeyString.Buffer, Status);
            return ERROR_GEN_FAILURE;
        }

        ParentCellOffset = BlockOffset;
        if (End)
            LocalKeyName = End + 1;
        else
            break;
    }

    CurrentKey = CreateInMemoryStructure(ParentRegistryHive, ParentCellOffset);
    if (!CurrentKey)
        return ERROR_NOT_ENOUGH_MEMORY;

    *Key = MEMKEY_TO_REGHKEY(CurrentKey);

    return ERROR_SUCCESS;
}
