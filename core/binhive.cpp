#define NDEBUG

#include <stdio.h>
#include <stdlib.h>

#include "typedefs.hpp"
#include "cmlib.hpp"
#include "reg.hpp"

#define MAX_PATH 260


/**
 * @brief 导出二进制Hive文件。
 *
 * 该函数用于将注册表Hive导出为二进制文件。
 *
 * @param FileName 导出的文件名。
 * @param CmHive 要导出的注册表Hive。
 * @return BOOL 返回操作的结果。如果成功，返回TRUE；否则返回FALSE。
 */
BOOL
ExportBinaryHive(
    const char* FileName,
    PCMHIVE CmHive)
{
    FILE *File;
    BOOL ret;

    DPRINT("  Creating binary hive: %s\n", FileName);

    /* 创建新的Hive文件 */
    File = fopen(FileName, "wb");
    if (File == NULL)
    {
        DPRINT1("    Error creating/opening file\n");
        return FALSE;
    }

    fseek(File, 0, SEEK_SET);

    CmHive->FileHandles[HFILE_TYPE_PRIMARY] = File;
    ret = HvWriteHive(&CmHive->Hive);
    fclose(File);
    return ret;
}


/**
 * @brief 保存注册表数据到储巢文件
 * 
 * @param HivePath 储巢文件路径
 * @param HiveList 储巢文件列表
 * @param UpperCaseFileName 文件大小写
 *
 * @return 
 */
BOOLEAN
SaveRegistryIntoHive(char* HivePath, 
                    const char * HiveList,
                    BOOL UpperCaseFileName)
{
    INT i;
    char* ptr;
    char DestPath[PATH_MAX];

    if(HivePath == NULL) 
    {
        strcpy(DestPath, "Config");
    }else{
        strcpy(DestPath, HivePath);
    }
    
    char FileName[PATH_MAX];

    for (i = 0; i < MAX_NUMBER_OF_REGISTRY_HIVES; ++i)
    {
        /* 如果此注册表配置单元不在列表中则跳过它 */
        if (!strstr(HiveList, RegistryHives[i].HiveName))
            continue; 

        strcpy(FileName, DestPath);
        strcat(FileName, DIR_SEPARATOR_STRING);

        ptr = FileName + strlen(FileName);

        strcat(FileName, RegistryHives[i].HiveName);

        /* 特殊安装注册表配置单元的异常 */
        // if (strcmp(RegistryHives[i].HiveName, "SETUPREG") == 0)
        if (i == 0)
            strcat(FileName, ".HIV");

        /* 如果需要，调整文件名大小写 */
        if (UpperCaseFileName)
        {
            for (; *ptr; ++ptr)
                *ptr = toupper(*ptr);
        }
        else
        {
            for (; *ptr; ++ptr)
                *ptr = tolower(*ptr);
        }

        if (!ExportBinaryHive(FileName, RegistryHives[i].CmHive))
            return FALSE;

        /* 如果我们碰巧要处理特殊的设置注册表配置单元 就到此为止 */
        // if (strcmp(RegistryHives[i].HiveName, "SETUPREG") == 0)
        if (i == 0)
            break;
    }

    return TRUE;
}
