/*
 *  NTREG kernel
 *  Copyright (C) 2024 NTSoft
 *
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG hive maker
 * FILE:            
 * PURPOSE:         Hive maker
 * PROGRAMMERS:     Lily King
 */

/* INCLUDES *****************************************************************/

#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "mkhive.hpp"

#ifdef _MSC_VER
#include <stdlib.h>
#define PATH_MAX _MAX_PATH
#endif // _MSC_VER

#ifndef _WIN32
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif

/* FUNCTIONS ****************************************************************/

void usage(void)
{
    printf("Loong Data Binary hive maker\n");
    printf("Copyright (C) 2024 Lily King.\n\n");
    printf("Usage: mkhive [-?] -h:hive1[,hiveN...] [-u] -d:<dstdir> <inffiles>\n\n"
           "  -h:hiveN  - Comma-separated list of hives to create. Possible values are:\n"
           "              SETUPREG, SYSTEM, SOFTWARE, DEFAULT, SAM, SECURITY, BCD.\n"
           "  -u        - Generate file names in uppercase (default: lowercase) (TEMPORARY FLAG!).\n"
           "  -d:dstdir - The binary hive files are created in this directory.\n"
           "  inffiles  - List of INF files with full path.\n"
           "  -?        - Displays this help screen.\n");
}

void convert_path(char *dst, char *src)
{
    int i;

    i = 0;
    while (src[i] != 0)
    {
#ifdef _WIN32
        if (src[i] == '/')
        {
            dst[i] = '\\';
        }
#else
        if (src[i] == '\\')
        {
            dst[i] = '/';
        }
#endif
        else
        {
            dst[i] = src[i];
        }

        i++;
    }
    dst[i] = 0;
}

int main(int argc, char *argv[])
{
    INT ret;
    INT i;
    char* ptr;
    BOOL UpperCaseFileName = FALSE; /* 输出Hive文件是否大写 */
    const char * HiveList = NULL;
    CHAR DestPath[PATH_MAX] = "";
    CHAR FileName[PATH_MAX];

    if (argc < 4)
    {
        usage();
        return -1;
    }

    printf("Binary hive maker\n");

    /* 分析选项 */
    for (i = 1; i < argc && *argv[i] == '-'; i++)
    {
        if (argv[i][1] == '?' && argv[i][2] == 0)
        {
            usage();
            return 0;
        }

        if (argv[i][1] == 'u' && argv[i][2] == 0)
        {
            UpperCaseFileName = TRUE;
        }
        else
        if (argv[i][1] == 'h' && (argv[i][2] == ':' || argv[i][2] == '='))
        {
            HiveList = argv[i] + 3;
        }
        else if (argv[i][1] == 'd' && (argv[i][2] == ':' || argv[i][2] == '='))
        {
            convert_path(DestPath, argv[i] + 3);
        }
        else
        {
            fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
            return -1;
        }
    }

    /* 检查我们是否拥有所需的所有参数 */
    if (!HiveList || !*HiveList)
    {
        fprintf(stderr, "The mandatory list of hives is missing.\n");
        return -1;
    }
    if (!*DestPath)
    {
        fprintf(stderr, "The mandatory output directory is missing.\n");
        return -1;
    }
    if (i >= argc)
    {
        fprintf(stderr, "Not enough parameters, or the list of INF files is missing.\n");
        return -1;
    }

    /* 初始化注册表 */
    RegInitializeRegistry(HiveList, FALSE);

    /* 默认为失败 */
    ret = -1;

    /* 现在我们应该有INF文件的列表：解析它 */
    for (; i < argc; ++i)
    {
        convert_path(FileName, argv[i]);
        if (!ImportRegistryFile(FileName))
            goto Quit;
    }

    if(SaveRegistryIntoHive(DestPath, HiveList, UpperCaseFileName))
    {
        /* Success */
        ret = 0;
    }

Quit:
    /* Shut down the registry */
    // RegShutdownRegistry();

    if (ret == 0)
        printf("  Done.\n");

    return ret;
}

/* EOF */
