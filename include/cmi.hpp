/*
 *  NTREG kernel
 *  Copyright (C) 2024 NTSoft
 *
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG hive maker
 * FILE:            cmi.h
 * PURPOSE:         Registry file manipulation routines
 * PROGRAMMER:      Lily King
 */

#ifndef _CMI_H_
#define _CMI_H_

#include "typedefs.hpp"
#include "cmlib.hpp"

#define VERIFY_KEY_CELL(key)

int
CmiInitializeHive(
    IN OUT PCMHIVE Hive,
    IN PCWSTR Name);

int
CmiCreateSecurityKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN unsigned char* Descriptor,
    IN ULONG DescriptorLength);

int
CmiAddSubKey(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX ParentKeyCellOffset,
    IN PCUNICODE_STRING SubKeyName,
    IN BOOLEAN VolatileKey,
    OUT HCELL_INDEX *pBlockOffset);

int
CmiAddValueKey(
    IN PCMHIVE RegistryHive,
    IN PCM_KEY_NODE Parent,
    IN ULONG ChildIndex,
    IN PCUNICODE_STRING ValueName,
    OUT PCM_KEY_VALUE *pValueCell,
    OUT HCELL_INDEX *pValueCellOffset);


/********** Runtime Function **********/
void*
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag);

void
CmpFree(
    IN void* Ptr,
    IN ULONG Quota);

BOOLEAN
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    OUT void* Buffer,
    IN SIZE_T BufferLength);

BOOLEAN
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    IN void* Buffer,
    IN SIZE_T BufferLength);

BOOLEAN
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize);


BOOLEAN
CmpFileFlush(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length);

#endif // _CMI_H_
