/*
 * PROJECT:   Registry Library
 * FILE:      reg_hive_mgmt.cpp
 * PURPOSE:   Hive management (connect, load, initialize, shutdown, InitNTReg)
 */

#include <stdio.h>
#include <stdlib.h>

#define NDEBUG

#include "Pal.hpp"
#include "typedefs.hpp"
#include "cmlib.hpp"
#include "reg.hpp"
#include "cmi.hpp"
#include "binhive.hpp"
#include "reg_internal.hpp"

static BOOL
ConnectRegistry(
    IN REGHKEY RootKey,
    IN PCWSTR Path,
    IN PCMHIVE HiveToConnect,
    IN unsigned char* SecurityDescriptor,
    IN ULONG SecurityDescriptorLength)
{
    int Status;
    LONG rc;
    PREPARSE_POINT ReparsePoint;
    PMEMKEY NewKey;

    ReparsePoint = static_cast<PREPARSE_POINT>(malloc(sizeof(*ReparsePoint)));
    if (!ReparsePoint)
        return FALSE;

    Status = CmiInitializeHive(HiveToConnect, u"$$PROTO.HIV");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed with status 0x%08x\n", Status);
        free(ReparsePoint);
        return FALSE;
    }

    Status = CmiCreateSecurityKey(&HiveToConnect->Hive,
                                  HiveToConnect->Hive.BaseBlock->RootCell,
                                  SecurityDescriptor, SecurityDescriptorLength);
    if (!NT_SUCCESS(Status))
        DPRINT1("Failed to add security for root key '%S'\n", Path);

    rc = NTRegCreateKeyExW(RootKey,
                         Path,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         0,
                         NULL,
                         reinterpret_cast<PREGHKEY>(&NewKey),
                         NULL);
    if (rc != ERROR_SUCCESS)
    {
        free(ReparsePoint);
        return FALSE;
    }

    ReparsePoint->SourceHive = NewKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = NewKey->KeyCellOffset;
    NewKey->RegistryHive = HiveToConnect;
    NewKey->KeyCellOffset = HiveToConnect->Hive.BaseBlock->RootCell;
    ReparsePoint->DestinationHive = NewKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = NewKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);

    return TRUE;
}

static BOOL
CreateSymLink(
    IN PCWSTR LinkKeyPath OPTIONAL,
    IN OUT PREGHKEY LinkKeyHandle OPTIONAL,
    IN REGHKEY TargetKeyHandle)
{
    LONG rc;
    PMEMKEY LinkKey, TargetKey;
    PREPARSE_POINT ReparsePoint;

    ReparsePoint = static_cast<PREPARSE_POINT>(malloc(sizeof(*ReparsePoint)));
    if (!ReparsePoint)
        return FALSE;

    if (LinkKeyPath && !(LinkKeyHandle && *LinkKeyHandle))
    {
        rc = NTRegCreateKeyExW(NULL,
                             LinkKeyPath,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             0,
                             NULL,
                             reinterpret_cast<PREGHKEY>(&LinkKey),
                             NULL);
        if (rc != ERROR_SUCCESS)
        {
            free(ReparsePoint);
            return FALSE;
        }
    }
    else if (LinkKeyHandle)
    {
        LinkKey = REGHKEY_TO_MEMKEY(*LinkKeyHandle);
    }

    if (LinkKeyHandle)
        *LinkKeyHandle = MEMKEY_TO_REGHKEY(LinkKey);

    TargetKey = REGHKEY_TO_MEMKEY(TargetKeyHandle);

    ReparsePoint->SourceHive = LinkKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = LinkKey->KeyCellOffset;
    ReparsePoint->DestinationHive = TargetKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = TargetKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);

    return TRUE;
}

LONG CMAPI
LoadHiveDataIntoRegistry(
    _In_ PCMHIVE CmHive,
    _In_ char* FileName)
{
    int Status;
    PCM_KEY_NODE RootKeyNode;
    CM_CHECK_REGISTRY_STATUS CmStatusCode;

    Status = HvInitialize(&CmHive->Hive,
                          HINIT_FILE,
                          HIVE_NOLAZYFLUSH,
                          HFILE_TYPE_PRIMARY,
                          NULL,
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
        return ERROR_GEN_FAILURE;
    }

    CmStatusCode = CmCheckRegistry(CmHive, CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES | CM_CHECK_REGISTRY_VALIDATE_HIVE);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        return ERROR_GEN_FAILURE;
    }

    RootKeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(&CmHive->Hive, CmHive->Hive.BaseBlock->RootCell));
    if (RootKeyNode)
        HvReleaseCell(&CmHive->Hive, CmHive->Hive.BaseBlock->RootCell);

    DPRINT1("LoadHiveDataIntoRegistry done\n");
    return ERROR_SUCCESS;
}

BOOLEAN
RegInitializeRegistry(
    IN const char * HiveList,
    IN BOOLEAN LoadHiveData)
{
    int Status;
    UINT i;
    REGHKEY ControlSetKey;
    REGHKEY TempKey;

    InitializeListHead(&CmiHiveListHead);
    InitializeListHead(&CmiReparsePointsHead);

    g_ActiveHiveList = HiveList;

    Status = CmiInitializeHive(&RootHive, u"");
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed with status 0x%08x\n", Status);
        return FALSE;
    }

    RootKey = CreateInMemoryStructure(&RootHive,
                                      RootHive.Hive.BaseBlock->RootCell);

    for (i = 0; i < MAX_NUMBER_OF_REGISTRY_HIVES; ++i)
    {
        if (!strstr(HiveList, RegistryHives[i].HiveName))
            continue;

        ConnectRegistry(NULL,
                        RegistryHives[i].HiveRegistryPath,
                        RegistryHives[i].CmHive,
                        RegistryHives[i].SecurityDescriptor,
                        RegistryHives[i].SecurityDescriptorLength);

        if (i == 0)
            break;

        if (LoadHiveData == TRUE)
        {
            CHAR FileName[PATH_MAX];

            strcpy(FileName, Get_CurDir());
            strcat(FileName, DIR_SEPARATOR_STRING);

            strcat(FileName, HivePath);
            strcat(FileName, DIR_SEPARATOR_STRING);
            strcat(FileName, RegistryHives[i].HiveName);

            DPRINT1("Open Registry File: %s\n", FileName);

            FILE *Hivefile = fopen(FileName, "rb+");
            if (Hivefile == NULL)
            {
                DPRINT1("Could not find the hive: %s\n", FileName);
                printf("Open Registry Fail!\n");
                return FALSE;
            }
            DPRINT("Hivefile: %p\n", Hivefile);

            fseek(Hivefile, 0, SEEK_SET);

            RegistryHives[i].CmHive->FileHandles[HFILE_TYPE_PRIMARY] = Hivefile;

            if (LoadHiveDataIntoRegistry(RegistryHives[i].CmHive, const_cast<char*>(RegistryHives[i].HiveName)) == ERROR_SUCCESS)
            {
                DPRINT1("Load Hive data into the registry success\n");

            } else {
                DPRINT1("Load Hive data into the registry failed\n");
                return FALSE;
            }
        }
    }

#if 1
    NTRegCreateKeyW(NULL,
                  u"NTReg\\Local\\SYSTEM\\ControlSet001",
                  &ControlSetKey);

    CreateSymLink(u"NTReg\\Local\\SYSTEM\\CurrentControlSet",
                  NULL, ControlSetKey);

    NTRegCloseKey(ControlSetKey);
#endif
    NTRegCreateKeyW(NULL,
                  u"NTReg\\Local\\TEMP",
                  &TempKey);

    NTRegCloseKey(TempKey);

    return TRUE;
}

void
RegShutdownRegistry()
{
    PLIST_ENTRY Entry;
    PREPARSE_POINT ReparsePoint;
    UINT i;

    for (i = 0; i < MAX_NUMBER_OF_REGISTRY_HIVES; ++i)
    {
        if (!strstr(g_ActiveHiveList ? g_ActiveHiveList : HiveList, RegistryHives[i].HiveName))
            continue;

        if (i == 0)
            break;

        BOOLEAN status = HvSyncHive(&RegistryHives[i].CmHive->Hive);
        if (status == TRUE)
        {
            DPRINT1("%s\n", RegistryHives[i].HiveName);
        }else{
            DPRINT1("HvSyncHive() failed for %s\n", RegistryHives[i].HiveName);
        }
    }

    while (!IsListEmpty(&CmiReparsePointsHead))
    {
        Entry = RemoveHeadList(&CmiReparsePointsHead);
        ReparsePoint = CONTAINING_RECORD(Entry, REPARSE_POINT, ListEntry);
        free(ReparsePoint);
    }

    if (RootKey)
    {
        free(RootKey);
        RootKey = nullptr;
    }
}

BOOLEAN
NTRegConnect()
{
    int Status;

    if (!HiveList || !*HiveList)
    {
        fprintf(stderr, "The mandatory list of hives is missing.\n");
        return FALSE;
    }

    Status = RegInitializeRegistry(HiveList, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RegInitializeRegistry() failed with status 0x%08x\n", Status);
        return FALSE;
    }

    NTRegOpenKeyW(NULL, u"NTReg", &REGHKEY_ROOT);
    NTRegOpenKeyW(NULL, u"NTReg\\Local", &REGHKEY_LOCAL);

    NTRegOpenKeyW(NULL, u"NTReg\\Local\\SYSTEM", &REGHKEY_SYSTEM);
    NTRegOpenKeyW(NULL, u"NTReg\\Local\\SOFTWARE", &REGHKEY_SOFTWARE);

    NTRegOpenKeyW(NULL, u"NTReg\\Local\\SYSTEM\\NTSoft\\NTReg\\CurrentVersion", &REGHKEY_CURRENT_VERSION);

    return TRUE;
}

void
NTRegDisconnect()
{
    RegShutdownRegistry();
}

NTReg* InitNTReg()
{
    NTReg* interface = static_cast<NTReg*>(malloc(sizeof(NTReg)));
    if (!interface)
        return nullptr;
    interface->Connect = NTRegConnect;
    interface->Disconnect = NTRegDisconnect;

    interface->OpenKey = NTRegOpenKeyW;
    interface->CloseKey = NTRegCloseKey;

    interface->CreateKey = NTRegCreateKeyW;
    interface->CreateKeyEx = NTRegCreateKeyExW;
    interface->DeleteKey = NTRegDeleteKeyW;
    interface->EnumKey = NTRegEnumKey;

    interface->QueryValue = NTRegQueryValueExW;
    interface->SetValue = NTRegSetValueExW;
    interface->DeleteValue = NTRegDeleteValueW;
    interface->EnumValue = NTRegEnumValue;

    return interface;
}
