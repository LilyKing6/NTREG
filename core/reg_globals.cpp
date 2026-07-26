/*
 * PROJECT:   Registry Library
 * FILE:      reg_globals.cpp
 * PURPOSE:   Global state definitions for the registry module
 */

#include <stdlib.h>

#define NDEBUG

#include "Pal.hpp"
#include "typedefs.hpp"
#include "cmlib.hpp"
#include "reg.hpp"
#include "cmi.hpp"
#include "binhive.hpp"
#include "security_data.hpp"
#include "reg_internal.hpp"

/* Boot-time hive list */
const char* const HiveList = "SYSTEM";

/* Active hive list set by RegInitializeRegistry (used by RegShutdownRegistry) */
const char* g_ActiveHiveList = nullptr;

REGHKEY REGHKEY_ROOT;
REGHKEY REGHKEY_LOCAL;
REGHKEY REGHKEY_SYSTEM;
REGHKEY REGHKEY_SOFTWARE;
REGHKEY REGHKEY_CURRENT_VERSION;

CHAR HivePath[PATH_MAX] = "Config";

CMHIVE RootHive;
PMEMKEY RootKey;

static CMHIVE SystemHive;
static CMHIVE SoftwareHive;
static CMHIVE DefaultHive;
static CMHIVE SamHive;
static CMHIVE SecurityHive;
static CMHIVE BcdHive;

HIVE_LIST_ENTRY RegistryHives[] =
{
    { "SETUPREG", u"NTReg\\Local\\SYSTEM"     , &SystemHive  , SystemSecurity  , SystemSecuritySize   },
    { "SYSTEM"  , u"NTReg\\Local\\SYSTEM"     , &SystemHive  , SystemSecurity  , SystemSecuritySize   },
    { "SOFTWARE", u"NTReg\\Local\\SOFTWARE"   , &SoftwareHive, SoftwareSecurity, SoftwareSecuritySize },
    { "DEFAULT" , u"NTReg\\User\\.DEFAULT"      , &DefaultHive , SystemSecurity  , SystemSecuritySize   },
};
C_ASSERT(_countof(RegistryHives) == MAX_NUMBER_OF_REGISTRY_HIVES);

LIST_ENTRY CmiHiveListHead;
LIST_ENTRY CmiReparsePointsHead;

PMEMKEY
CreateInMemoryStructure(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX KeyCellOffset)
{
    PMEMKEY Key;

    Key = static_cast<PMEMKEY>(malloc(sizeof(MEMKEY)));
    if (!Key)
        return NULL;

    Key->RegistryHive = RegistryHive;
    Key->KeyCellOffset = KeyCellOffset;
    return Key;
}
