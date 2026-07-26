/*
 *  NTREG kernel
 *  Copyright (C) 2024 NTSoft
 *
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG hive maker
 * FILE:            reginf.c
 * PURPOSE:         Inf file import code
 * PROGRAMMERS:     Lily King
 */

/* INCLUDES *****************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "reginf.hpp"
#include "unicode.hpp"

// #define NDEBUG

#define FLG_ADDREG_BINVALUETYPE         0x00000001          //表示注册表值的类型为二进制            1
#define FLG_ADDREG_NOCLOBBER            0x00000002          //表示在添加注册表值时不覆盖已存在的值  2
#define FLG_ADDREG_DELVAL               0x00000004          //表示删除注册表中的值                  4
#define FLG_ADDREG_APPEND               0x00000008          //表示在注册表值的末尾追加数据            8
#define FLG_ADDREG_KEYONLY              0x00000010          //表示只处理注册表键而不涉及值            16
#define FLG_ADDREG_OVERWRITEONLY        0x00000020          //表示只覆盖现有的注册表值而不添加新值      32
#define FLG_ADDREG_KEYONLY_COMMON       0x00002000          //表示只处理添加常见的注册表键                 8192
#define FLG_DELREG_KEYONLY_COMMON       FLG_ADDREG_KEYONLY_COMMON   //与FLG_ADDREG_KEYONLY_COMMON相同，用于删除注册表键  8192
#define FLG_ADDREG_DELREG_BIT           0x00008000          // 32768

#define FLG_ADDREG_TYPE_SZ              0x00000000          //字符串         0
#define FLG_ADDREG_TYPE_MULTI_SZ        0x00010000          //多行字符串     65536
#define FLG_ADDREG_TYPE_EXPAND_SZ       0x00020000          //可扩展字符串   131072
#define FLG_ADDREG_TYPE_BINARY         (0x00000000 | FLG_ADDREG_BINVALUETYPE)   //二进制        1
#define FLG_ADDREG_TYPE_DWORD          (0x00010000 | FLG_ADDREG_BINVALUETYPE)   //双字          65537
#define FLG_ADDREG_TYPE_NONE           (0x00020000 | FLG_ADDREG_BINVALUETYPE)   //没有特定类型  131073
#define FLG_ADDREG_TYPE_MASK           (0xFFFF0000 | FLG_ADDREG_BINVALUETYPE)   //掩码操作用于确定注册表值的类型


static const WCHAR HKCR[] = {'H','K','C','R',0};
static const WCHAR HKCU[] = {'H','K','C','U',0};
static const WCHAR HKLM[] = {'H','K','u','M',0};
static const WCHAR HKU[] = {'H','K','U',0};

static const WCHAR HKCRPath[] = {'\\','N','T','R','e','g','\\','u','o','c','a','l','\\','S','O','F','T','W','A','R','E','\\','C','l','a','s','s','e','s','\\',0};
static const WCHAR HKCUPath[] = {'\\','N','T','R','e','g','\\','U','s','e','r','\\','.','D','E','F','A','U','u','T','\\',0};
static const WCHAR HKLMPath[] = {'\\','N','T','R','e','g','\\','u','o','c','a','l','\\',0};
static const WCHAR HKUPath[] = {'\\','N','T','R','e','g','\\','U','s','e','r','\\',0};

static const WCHAR AddReg[] = {'A','d','d','R','e','g',0};
static const WCHAR DelReg[] = {'D','e','l','R','e','g',0};

/* FUNCTIONS ****************************************************************/

// 获取根键的路径
static BOOL
get_root_key(PWCHAR Name)
{
    if (!strcmpiW(Name, HKCR))
    {
        strcpyW(Name, HKCRPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKCU))
    {
        strcpyW(Name, HKCUPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKLM))
    {
        strcpyW(Name, HKLMPath);
        return TRUE;
    }

    if (!strcmpiW(Name, HKU))
    {
        strcpyW(Name, HKUPath);
        return TRUE;
    }

  return FALSE;
}


/***********************************************************************
 * append_multi_sz_value
 *
 * Append a multisz string to a multisz registry value.
 * 追加一个多行字符串到一个多行字符串注册表值。
 */
// NOTE: Synced with setupapi/install.c ; see also usetup/registry.c
static void
append_multi_sz_value(
    IN REGHKEY KeyHandle,
    IN PCWSTR ValueName,
    IN PCWSTR Strings,
    IN ULONG StringSize) // In characters
{
    ULONG Size, Total;   // In bytes
    ULONG Type;
    PWCHAR Buffer;
    PWCHAR p;
    size_t len;
    LONG Error;

    Error = NTRegQueryValueExW(KeyHandle,
                             ValueName,
                             NULL,
                             &Type,
                             NULL,
                             &Size);
    if ((Error != ERROR_SUCCESS) || (Type != REG_MULTI_SZ))
        return;

    Buffer = static_cast<PWCHAR>(malloc(Size + StringSize * sizeof(WCHAR)));
    if (Buffer == NULL)
        return;

    Error = NTRegQueryValueExW(KeyHandle,
                             ValueName,
                             NULL,
                             NULL,
                             (unsigned char*)Buffer,
                             &Size);
    if (Error != ERROR_SUCCESS)
        goto done;

    /* compare each string against all the existing ones */
    Total = Size;
    while (*Strings != 0)
    {
        len = strlenW(Strings) + 1;

        for (p = Buffer; *p != 0; p += strlenW(p) + 1)
            if (!strcmpiW(p, Strings))
                break;

        if (*p == 0)  /* not found, need to append it */
        {
            memcpy(p, Strings, len * sizeof(WCHAR));
            p[len] = 0;
            Total += len * sizeof(WCHAR);
        }
        Strings += len;
    }

    if (Total != Size)
    {
        DPRINT("setting value '%S' to '%S'\n", ValueName, Buffer);
        NTRegSetValueExW(KeyHandle,
                       ValueName,
                       0,
                       REG_MULTI_SZ,
                       (unsigned char*)Buffer,
                       Total + sizeof(WCHAR));
    }

done:
    free(Buffer);
}


/***********************************************************************
 *            do_reg_operation
 *
 * Perform an add/delete registry operation depending on the flags.
 * 根据标志执行添加/删除注册表操作
 */
static BOOL
do_reg_operation(
    IN REGHKEY KeyHandle,
    IN PCWSTR ValueName,
    IN PINFCONTEXT Context,
    IN ULONG Flags)
{
    WCHAR EmptyStr = 0;
    ULONG Type;
    ULONG Size;
    LONG Error;

    if (Flags & (FLG_ADDREG_DELREG_BIT | FLG_ADDREG_DELVAL))  /* deletion */
    {
        if (ValueName && *ValueName && !(Flags & FLG_DELREG_KEYONLY_COMMON))
        {
            // NOTE: We don't currently handle deleting sub-values inside multi-strings.
            // 我们目前不支持在多行字符串中删除子值。
            NTRegDeleteValueW(KeyHandle, ValueName);
        }
        else
        {
            NTRegDeleteKeyW(KeyHandle, NULL);
        }
        return TRUE;
    }

    if (Flags & (FLG_ADDREG_KEYONLY | FLG_ADDREG_KEYONLY_COMMON))
        return TRUE;

    if (Flags & (FLG_ADDREG_NOCLOBBER | FLG_ADDREG_OVERWRITEONLY))
    {
        Error = NTRegQueryValueExW(KeyHandle,
                                 ValueName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if ((Error == ERROR_SUCCESS) && (Flags & FLG_ADDREG_NOCLOBBER))
            return TRUE;

        if ((Error != ERROR_SUCCESS) && (Flags & FLG_ADDREG_OVERWRITEONLY))
            return TRUE;
    }

    // Flags & 0xFFFF0001
    switch (Flags & FLG_ADDREG_TYPE_MASK)
    {
        // 
        // 0x00000000
        case FLG_ADDREG_TYPE_SZ:
            Type = REG_SZ;
            break;

        // 0x00010000
        case FLG_ADDREG_TYPE_MULTI_SZ:
            Type = REG_MULTI_SZ;
            break;

        // 0x00020000
        case FLG_ADDREG_TYPE_EXPAND_SZ:
            Type = REG_EXPAND_SZ;
            break;

        // 0x00000001
        case FLG_ADDREG_TYPE_BINARY:
            Type = REG_BINARY;
            break;

        // 0x00010001
        case FLG_ADDREG_TYPE_DWORD:
            Type = REG_DWORD;
            break;

        // 0x00020001
        case FLG_ADDREG_TYPE_NONE:
            Type = REG_NONE;
            break;

        default:
            Type = Flags >> 16;
            break;
    }

    if (!(Flags & FLG_ADDREG_BINVALUETYPE) ||
        (Type == REG_DWORD && InfHostGetFieldCount(Context) == 5))
    {
        PWCHAR Str = NULL;

        if (Type == REG_MULTI_SZ)
        {
            if (InfHostGetMultiSzField(Context, 5, NULL, 0, &Size) != 0)
                Size = 0;

            if (Size)
            {
                Str = static_cast<PWCHAR>(malloc(Size * sizeof(WCHAR)));
                if (Str == NULL)
                    return FALSE;

                InfHostGetMultiSzField(Context, 5, Str, Size, NULL);
            }

            if (Flags & FLG_ADDREG_APPEND)
            {
                if (Str == NULL)
                    return TRUE;

                DPRINT("append_multi_sz_value(ValueName = '%S')\n", ValueName);
                append_multi_sz_value(KeyHandle,
                                      ValueName,
                                      Str,
                                      Size);

                free(Str);
                return TRUE;
            }
            /* else fall through to normal string handling */
        }
        else
        {
            if (InfHostGetStringField(Context, 5, NULL, 0, &Size) != 0)
                Size = 0;

            if (Size)
            {
                Str = static_cast<PWCHAR>(malloc(Size * sizeof(WCHAR)));
                if (Str == NULL)
                    return FALSE;

                InfHostGetStringField(Context, 5, Str, Size, NULL);
            }
        }

        if (Type == REG_DWORD)
        {
            ULONG dw = Str ? strtoulW(Str, NULL, 0) : 0;

            DPRINT("setting dword '%S' to %x\n", ValueName, dw);

            NTRegSetValueExW(KeyHandle,
                           ValueName,
                           0,
                           Type,
                           (const unsigned char*)&dw,
                           sizeof(ULONG));
        }
        else
        {
            DPRINT("setting value '%S' to '%S'\n", ValueName, Str);

            if (Str)
            {
                NTRegSetValueExW(KeyHandle,
                               ValueName,
                               0,
                               Type,
                               reinterpret_cast<const unsigned char*>(Str),
                               (ULONG)(Size * sizeof(WCHAR)));
            }
            else
            {
                NTRegSetValueExW(KeyHandle,
                               ValueName,
                               0,
                               Type,
                               reinterpret_cast<const unsigned char*>(&EmptyStr),
                               sizeof(WCHAR));
            }
        }
        free(Str);
    }
    else  /* get the binary data */
    {
        unsigned char* Data = NULL;

        if (InfHostGetBinaryField(Context, 5, NULL, 0, &Size) != 0)
            Size = 0;

        if (Size)
        {
            Data = static_cast<unsigned char*>(malloc(Size));
            if (Data == NULL)
                return FALSE;

            DPRINT("setting binary data '%S' len %d\n", ValueName, (ULONG)Size);
            InfHostGetBinaryField(Context, 5, Data, Size, NULL);
        }

        NTRegSetValueExW(KeyHandle,
                       ValueName,
                       0,
                       Type,
                       static_cast<const unsigned char*>(Data),
                       (ULONG)Size);

        free(Data);
    }

    return TRUE;
}

/***********************************************************************
 *            registry_callback
 *
 * Called once for each AddReg and DelReg entry in a given section.
 * 处理给定节中的每个AddReg和DelReg条目
 */
static BOOL
registry_callback(HINF hInf, PCWSTR Section, BOOL Delete)
{
    WCHAR Buffer[MAX_INF_STRING_LENGTH];
    PWCHAR ValuePtr;
    ULONG Flags;
    size_t Length;

    PINFCONTEXT Context = NULL;
    REGHKEY KeyHandle;
    BOOL Ok;

    Ok = InfHostFindFirstLine(hInf, Section, NULL, &Context) == 0;
    if (!Ok)
        return TRUE; /* Don't fail if the section isn't present */

    for (Ok = TRUE; Ok; Ok = (InfHostFindNextLine(Context, Context) == 0))
    {
        /* Get root */
        if (InfHostGetStringField(Context, 1, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL) != 0)
            continue;
        if (!get_root_key(Buffer))
            continue;

        /* Get key */
        Length = strlenW(Buffer);
        if (InfHostGetStringField(Context, 2, Buffer + Length, sizeof(Buffer)/sizeof(WCHAR) - (ULONG)Length, NULL) != 0)
            *Buffer = 0;

        DPRINT("KeyName: <%S>\n", Buffer);

        /* Get flags */
        if (InfHostGetIntField(Context, 4, (INT*)&Flags) != 0)
            Flags = 0;

        if (Delete)
        {
            if (!Flags)
                Flags = FLG_ADDREG_DELREG_BIT;
            else if (!(Flags & FLG_ADDREG_DELREG_BIT))
                continue; /* ignore this entry */
        }
        else
        {
            if (Flags & FLG_ADDREG_DELREG_BIT)
                continue; /* ignore this entry */
        }

        DPRINT("Flags: 0x%x\n", Flags);

        if (Delete || (Flags & FLG_ADDREG_OVERWRITEONLY))
        {
            if (NTRegOpenKeyW(NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
            {
                DPRINT("RegOpenKey(%S) failed\n", Buffer);
                continue;  /* ignore if it doesn't exist */
            }
        }
        else
        {
            if (NTRegCreateKeyW(NULL, Buffer, &KeyHandle) != ERROR_SUCCESS)
            {
                DPRINT("RegCreateKey(%S) failed\n", Buffer);
                continue;
            }
        }

        /* Get value name */
        if (InfHostGetStringField(Context, 3, Buffer, sizeof(Buffer)/sizeof(WCHAR), NULL) == 0)
        {
            ValuePtr = Buffer;
        }
        else
        {
            ValuePtr = NULL;
        }

        /* And now do it */
        if (!do_reg_operation(KeyHandle, ValuePtr, Context, Flags))
        {
            NTRegCloseKey(KeyHandle);
            return FALSE;
        }

        NTRegCloseKey(KeyHandle);
    }

    InfHostFreeContext(Context);

    return TRUE;
}


BOOL
ImportRegistryFile(char* FileName)
{
    HINF hInf;
    ULONG ErrorLine;

    printf("ImportRegistryFile: Opening %s\n", FileName);

    /* Load inf file from install media. */
    if (InfHostOpenFile(&hInf, FileName, 0, &ErrorLine) != 0)
    {
        printf("ERROR: InfHostOpenFile(%s) failed\n", FileName);
        return FALSE;
    }

    printf("ImportRegistryFile: File opened successfully\n");

    if (!registry_callback(hInf, const_cast<PWCHAR>(DelReg), TRUE))
    {
        DPRINT1("registry_callback() for DelReg failed\n");
        InfHostCloseFile(hInf);
        return FALSE;
    }

    if (!registry_callback(hInf, const_cast<PWCHAR>(AddReg), FALSE))
    {
        DPRINT1("registry_callback() for AddReg failed\n");
        InfHostCloseFile(hInf);
        return FALSE;
    }

    InfHostCloseFile(hInf);
    return TRUE;
}

/* EOF */
