/*
 * PROJECT:   Registry Library
 * FILE:      reg_keyops.cpp
 * PURPOSE:   Registry key operations (open, close, create, delete, enum)
 */

#define NDEBUG

#include "cmlib.hpp"
#include "reg_internal.hpp"

LONG
NTRegCloseKey(
    IN REGHKEY hKey)
{
    PMEMKEY Key = REGHKEY_TO_MEMKEY(hKey);

    free(Key);

    return ERROR_SUCCESS;
}

LONG
NTRegCreateKeyW(
    IN REGHKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PREGHKEY phkResult)
{
    return RegpCreateOrOpenKey(hKey, lpSubKey, TRUE, FALSE, phkResult);
}

LONG
NTRegCreateKeyExW(
    IN REGHKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN void* lpSecurityAttributes OPTIONAL,
    OUT PREGHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL)
{
    return RegpCreateOrOpenKey(hKey,
                               lpSubKey,
                               TRUE,
                               (dwOptions & REG_OPTION_VOLATILE) != 0,
                               phkResult);
}

LONG
NTRegDeleteKeyW(
    IN REGHKEY hKey,
    IN LPCWSTR lpSubKey)
{
    LONG rc;
    int Status;
    REGHKEY hTargetKey;
    PMEMKEY Key;
    PHHIVE Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_NODE Parent;
    HCELL_INDEX ParentCell;

    if (lpSubKey)
    {
        rc = NTRegOpenKeyW(hKey, lpSubKey, &hTargetKey);
        if (rc != ERROR_SUCCESS)
            return rc;
    }
    else
    {
        hTargetKey = hKey;
        rc = ERROR_SUCCESS;
    }

    if (hTargetKey == RootKey)
    {
        rc = ERROR_ACCESS_DENIED;
        goto Quit;
    }

    Key = REGHKEY_TO_MEMKEY(hTargetKey);
    Hive = &Key->RegistryHive->Hive;

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Key->KeyCellOffset));
    if (!KeyNode)
    {
        rc = ERROR_GEN_FAILURE;
        goto Quit;
    }

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if (!(KeyNode->SubKeyCounts[Stable] + KeyNode->SubKeyCounts[Volatile]) &&
        !(KeyNode->Flags & KEY_NO_DELETE))
    {
        ParentCell = KeyNode->Parent;
        Status = CmpFreeKeyByCell(Hive, Key->KeyCellOffset, TRUE);
        if (NT_SUCCESS(Status))
        {
            Parent = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, ParentCell));
            if (Parent)
            {
                ASSERT(HvIsCellDirty(Hive, ParentCell));
                KeQuerySystemTime(&Parent->LastWriteTime);
                HvReleaseCell(Hive, ParentCell);
            }

            rc = ERROR_SUCCESS;
        }
        else
        {
            rc = ERROR_GEN_FAILURE;
        }
    }
    else
    {
        rc = ERROR_ACCESS_DENIED;
    }

    HvReleaseCell(Hive, Key->KeyCellOffset);

Quit:
    if (lpSubKey)
        NTRegCloseKey(hTargetKey);

    return rc;
}

LONG
NTRegEnumKey(
    IN REGHKEY Key,
    IN ULONG Index,
    OUT PWCHAR Name,
    IN OUT PULONG NameSize,
    OUT PREGHKEY SubKey OPTIONAL)
{
    PMEMKEY ParentKey = REGHKEY_TO_MEMKEY(Key);
    PHHIVE Hive = &ParentKey->RegistryHive->Hive;

    PCM_KEY_NODE KeyNode, SubKeyNode;
    HCELL_INDEX CellIndex;
    USHORT NameLength;

    DPRINT("NTRegEnumKey(%p, %lu, %p, %p->%u)\n",
          Key, Index, Name, NameSize, NameSize ? *NameSize : 0);

    KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, ParentKey->KeyCellOffset));
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, Index);
    if (CellIndex == HCELL_NIL)
    {
        DPRINT("NTRegEnumKey index out of bounds (%d) in key (%.*s)\n",
              Index, KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, REGHKEY_TO_HCI(Key));
        return ERROR_NO_MORE_ITEMS;
    }
    HvReleaseCell(Hive, REGHKEY_TO_HCI(Key));

    SubKeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, CellIndex));
    ASSERT(SubKeyNode != NULL);
    ASSERT(SubKeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if (SubKeyNode->Flags & KEY_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(SubKeyNode->Name, SubKeyNode->NameLength);
        CmpCopyCompressedName(Name,
                              *NameSize,
                              SubKeyNode->Name,
                              SubKeyNode->NameLength);
    }
    else
    {
        NameLength = SubKeyNode->NameLength;
        RtlCopyMemory(Name, SubKeyNode->Name,
                      mininum(*NameSize, SubKeyNode->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    HvReleaseCell(Hive, CellIndex);

    if (SubKey != NULL)
        *SubKey = HCI_TO_REGHKEY(CellIndex);

    DPRINT("NTRegEnumKey done -> %u, '%.*S'\n", *NameSize, *NameSize, Name);
    return ERROR_SUCCESS;
}

LONG
NTRegOpenKeyW(
    IN REGHKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PREGHKEY phkResult)
{
    return RegpCreateOrOpenKey(hKey, lpSubKey, FALSE, FALSE, phkResult);
}
