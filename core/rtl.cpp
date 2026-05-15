/*
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG 
 * FILE:            rtl.c
 * PURPOSE:         Runtime Library
 */

#include <stdlib.h>
#include <stdarg.h>

/* gcc defaults to cdecl */
#if defined(__GNUC__)
#undef __cdecl
#define __cdecl
#endif

#include <stdio.h>
#include <stdlib.h>

#include "typedefs.hpp"

#include "cmlib.hpp"

#include "cmi.hpp"
#include "reg.hpp"


/**
 * @brief 初始化一个 UNICODE_STRING 结构。
 *
 * 该函数通过根据提供的源字符串设置其字段来初始化一个 UNICODE_STRING 结构。
 * 如果源字符串为 NULL，则假定源字符串的长度为 0。
 *
 * @param DestinationString 指向要初始化的 UNICODE_STRING 结构的指针。
 * @param SourceString 指向 Unicode 格式的源字符串的指针。
 *
 * @note 如果 SourceString 为 NULL，DestinationString 的 Length 和 MaximumLength 字段将被设置为 0。
 * @note DestinationString 的 Buffer 字段将指向 SourceString。
 */
void
RtlInitUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString)
{
    SIZE_T DestSize;

    if(SourceString)
    {
        DestSize = strlenW(SourceString) * sizeof(WCHAR);
        DestinationString->Length = (USHORT)DestSize;
        DestinationString->MaximumLength = (USHORT)DestSize + sizeof(WCHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = const_cast<PWCHAR>(SourceString);
}


/**
 * @brief 比较两个 UNICODE_STRING 结构。
 *
 * 该函数比较两个 UNICODE_STRING 结构，根据是否区分大小写返回比较结果。
 *
 * @param String1 指向第一个要比较的 UNICODE_STRING 结构的指针。
 * @param String2 指向第二个要比较的 UNICODE_STRING 结构的指针。
 * @param CaseInSensitive 一个布尔值，指示比较是否区分大小写。如果为 TRUE，则不区分大小写；如果为 FALSE，则区分大小写。
 *
 * @return 返回值表示比较结果：
 *         - 如果 String1 小于 String2，返回 -1。
 *         - 如果 String1 大于 String2，返回 1。
 *         - 如果 String1 等于 String2，返回 0。
 */
LONG 
RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive)
{
    USHORT i;
    WCHAR c1, c2;

    for (i = 0; i <= String1->Length / sizeof(WCHAR) && i <= String2->Length / sizeof(WCHAR); i++)
    {
        if (CaseInSensitive)
        {
            c1 = RtlUpcaseUnicodeChar(String1->Buffer[i]);
            c2 = RtlUpcaseUnicodeChar(String2->Buffer[i]);
        }
        else
        {
            c1 = String1->Buffer[i];
            c2 = String2->Buffer[i];
        }

        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;
    }

    return 0;
}


/**
 * @brief 将 Unicode 字符转换为大写形式。
 *
 * 该函数将输入的 Unicode 字符转换为大写形式。
 *
 * @param Source 要转换的 Unicode 字符。
 *
 * @return 返回转换后的 Unicode 字符。
 *
 * @note 如果输入字符小于 'a'，则直接返回原字符。
 * @note 如果输入字符在 'a' 和 'z' 之间，则返回相应的大写字符。
 * @note 对于其他字符，返回原字符。
 */
WCHAR 
RtlUpcaseUnicodeChar(
    IN WCHAR Source)
{
    USHORT Offset;

    if (Source < 'a')
        return Source;

    if (Source <= 'z')
        return (Source - ('a' - 'A'));

    Offset = 0;

    return Source + (SHORT)Offset;
}

/**
 * @brief 查询系统时间。
 *
 * 该函数查询当前系统时间并将其存储在提供的 LARGE_INTEGER 结构中。
 *
 * @param CurrentTime 指向存储当前系统时间的 LARGE_INTEGER 结构的指针。
 *
 * @note 该函数目前实现为将 CurrentTime 的 QuadPart 设置为 0。
 */
void 
KeQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime)
{
    CurrentTime->QuadPart = 0;
}

/**
 * @brief 分配内存池。
 *
 * 该函数根据指定的池类型和字节数分配内存池。
 *
 * @param PoolType 内存池的类型。
 * @param NumberOfBytes 要分配的字节数。
 *
 * @return 返回指向分配内存的指针，如果分配失败则返回 NULL。
 *
 * @note 该函数目前实现为使用 malloc 分配内存。
 */
void* 
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes)
{
    return malloc(NumberOfBytes);
}

/**
 * @brief 释放内存池。
 *
 * 该函数释放先前分配的内存池。
 *
 * @param p 指向要释放的内存的指针。
 *
 * @note 该函数目前实现为使用 free 释放内存。
 */
void 
ExFreePool(
    IN void* p)
{
    free(p);
}


/**
 * @brief 打印调试信息。
 *
 * 该函数用于打印调试信息，支持可变参数列表。
 *
 * @param Format 格式化字符串，指定输出的格式。
 * @param ... 可变参数列表，根据格式化字符串提供相应的参数。
 *
 * @return 返回 0 表示成功。
 *
 * @note 该函数目前实现为使用 vprintf 打印调试信息。
 */
ULONG
__cdecl
DbgPrint(
  IN CHAR *Format,
  IN ...)
{
    va_list ap;
    va_start(ap, Format);
    vprintf(Format, ap);
    va_end(ap);

    return 0;
}

/**
 * @brief 断言失败处理。
 *
 * 该函数用于处理断言失败的情况，打印断言失败的信息。
 *
 * @param FailedAssertion 指向失败断言的指针。
 * @param FileName 指向包含断言的文件名的指针。
 * @param LineNumber 断言失败的行号。
 * @param Message 可选的附加消息。
 *
 * @note 如果提供了 Message，则打印包含该消息的断言失败信息。
 * @note 该函数目前实现为使用 DPRINT 打印断言失败信息。
 */
void
RtlAssert(IN void* FailedAssertion,
          IN void* FileName,
          IN ULONG LineNumber,
          IN char* Message OPTIONAL)
{
    if (Message != NULL)
    {
        DPRINT("Assertion \'%s\' failed at %s line %u: %s\n",
                 const_cast<char*>(FailedAssertion),
                 const_cast<char*>(FileName),
                 LineNumber,
                 Message);
    }
    else
    {
        DPRINT("Assertion \'%s\' failed at %s line %u\n",
                 const_cast<char*>(FailedAssertion),
                 const_cast<char*>(FileName),
                 LineNumber);
    }

    //DbgBreakPoint();
}


/**
 * @brief 系统崩溃检查。
 *
 * 该函数用于处理系统崩溃的情况，打印崩溃信息并断言失败。
 *
 * @param BugCheckCode 崩溃检查代码。
 * @param BugCheckParameter1 崩溃检查参数1。
 * @param BugCheckParameter2 崩溃检查参数2。
 * @param BugCheckParameter3 崩溃检查参数3。
 * @param BugCheckParameter4 崩溃检查参数4。
 *
 * @note 该函数目前实现为打印崩溃信息并断言失败。
 */
void
KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4)
{
    printf("*** STOP: 0x%08X (0x%p,0x%p,0x%p,0x%p)",
           BugCheckCode,
           reinterpret_cast<void*>(BugCheckParameter1),
           reinterpret_cast<void*>(BugCheckParameter2),
           reinterpret_cast<void*>(BugCheckParameter3),
           reinterpret_cast<void*>(BugCheckParameter4));
    ASSERT(FALSE);
}

/**
 * @brief 查找第一个设置的位。
 *
 * 该函数查找给定掩码中第一个设置的位，并返回其索引。
 *
 * @param Index 指向存储第一个设置位的索引的指针。
 * @param Mask 要查找的位掩码。
 *
 * @return 如果找到设置的位，返回 1；否则返回 0。
 */
unsigned char BitScanForward(ULONG * Index, unsigned long Mask)
{
    *Index = 0;
    while (Mask && ((Mask & 1) == 0))
    {
        Mask >>= 1;
        ++(*Index);
    }
    return Mask ? 1 : 0;
}

/**
 * @brief 查找最后一个设置的位。
 *
 * 该函数查找给定掩码中最后一个设置的位，并返回其索引。
 *
 * @param Index 指向存储最后一个设置位的索引的指针。
 * @param Mask 要查找的位掩码。
 *
 * @return 如果找到设置的位，返回 1；否则返回 0。
 */
unsigned char BitScanReverse(ULONG * const Index, unsigned long Mask)
{
    *Index = 0;
    while (Mask && ((Mask & (1 << 31)) == 0))
    {
        Mask <<= 1;
        ++(*Index);
    }
    return Mask ? 1 : 0;
}

#include "unicode.hpp"
#include <cwctype>

int strcmpiW(const WCHAR *str1, const WCHAR *str2)
{
    while (*str1 && (::towlower(*str1) == ::towlower(*str2))) { str1++; str2++; }
    return ::towlower(*str1) - ::towlower(*str2);
}

unsigned long int strtoulW(const WCHAR *nptr, WCHAR **endptr, int base)
{
    unsigned long result = 0;
    const WCHAR *p = nptr;

    while (*p == L' ' || *p == L'\t') p++;

    if (base == 0) {
        if (*p == L'0') {
            p++;
            if (*p == L'x' || *p == L'X') { base = 16; p++; }
            else base = 8;
        } else base = 10;
    } else if (base == 16 && *p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
        p += 2;
    }

    while (*p) {
        int digit;
        if (*p >= L'0' && *p <= L'9') digit = *p - L'0';
        else if (*p >= L'a' && *p <= L'z') digit = *p - L'a' + 10;
        else if (*p >= L'A' && *p <= L'Z') digit = *p - L'A' + 10;
        else break;

        if (digit >= base) break;
        result = result * base + digit;
        p++;
    }

    if (endptr) *endptr = (WCHAR*)p;
    return result;
}

