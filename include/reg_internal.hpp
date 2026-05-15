/*
 * PROJECT:   Registry Library
 * FILE:      reg_internal.hpp
 * PURPOSE:   Internal types and shared state for reg module split
 */

#pragma once

#include "cmlib.hpp"
#include "reg.hpp"

/* Reparse point for registry symbolic links */
typedef struct _REPARSE_POINT
{
    LIST_ENTRY ListEntry;
    PCMHIVE SourceHive;
    HCELL_INDEX SourceKeyCellOffset;
    PCMHIVE DestinationHive;
    HCELL_INDEX DestinationKeyCellOffset;
} REPARSE_POINT, *PREPARSE_POINT;

/* In-memory registry key handle */
typedef struct _MEMKEY
{
    HCELL_INDEX KeyCellOffset;
    PCMHIVE RegistryHive;
} MEMKEY, *PMEMKEY;

#define REGHKEY_TO_MEMKEY(hKey) ((PMEMKEY)(hKey))
#define MEMKEY_TO_REGHKEY(memKey) ((REGHKEY)(memKey))

#define HCI_TO_REGHKEY(CellIndex)          ((REGHKEY)(ULONG_PTR)(CellIndex))
#ifndef REGHKEY_TO_HCI
#define REGHKEY_TO_HCI(hKey)               ((HCELL_INDEX)(ULONG_PTR)(hKey))
#endif

/* Shared global state (defined in reg_globals.cpp) */
extern const char* const HiveList;
extern REGHKEY REGHKEY_ROOT;
extern REGHKEY REGHKEY_LOCAL;
extern REGHKEY REGHKEY_SYSTEM;
extern REGHKEY REGHKEY_SOFTWARE;
extern REGHKEY REGHKEY_CURRENT_VERSION;
extern CHAR HivePath[PATH_MAX];
extern LIST_ENTRY CmiHiveListHead;
extern LIST_ENTRY CmiReparsePointsHead;
extern HIVE_LIST_ENTRY RegistryHives[];
extern const char* g_ActiveHiveList;

/* Helper: allocate and initialize a MEMKEY */
PMEMKEY CreateInMemoryStructure(IN PCMHIVE RegistryHive, IN HCELL_INDEX KeyCellOffset);

/* Helper: extract value data from a value cell */
void
RepGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE ValueCell,
    OUT PULONG Type OPTIONAL,
    OUT unsigned char* Data OPTIONAL,
    IN OUT PULONG DataSize OPTIONAL);

/* Root key (set by RegInitializeRegistry, used by path traversal) */
extern CMHIVE RootHive;
extern PMEMKEY RootKey;

/* Path walking engine */
int
RegpCreateOrOpenKey(
    IN REGHKEY hParentKey,
    IN PCWSTR KeyName,
    IN BOOL AllowCreation,
    IN BOOL Volatile,
    OUT PREGHKEY Key);
