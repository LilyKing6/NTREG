#pragma once

BOOL
ExportBinaryHive(
    const char * FileName,
    PCMHIVE CmHive);

LONG CMAPI
LoadHiveDataIntoRegistry(
    _In_ PCMHIVE CmHive,
    _In_ char* FileName);


void 
LoadRegistryHiveFile(char* HivePath, 
                    const char * HiveName,
                    char* *HiveBuffer,
                    PULONG HivefileSize);

BOOLEAN
SaveRegistryIntoHive(char* HivePath, 
                    const char * HiveList,
                    BOOL UpperCaseFileName);

