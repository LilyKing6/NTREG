/*
 * PROJECT:         NTREG Kernel
 * LICENSE:         See LICENSE in the top level directory
 * FILE:            cmlib/cmname.c
 * PURPOSE:         Configuration Manager - Name Management
 * PROGRAMMERS:     Lily King
 */

/* INCLUDES ******************************************************************/
#define NDEBUG

#include "cmlib.hpp"

#include "debug.hpp"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

/**
 * @brief
 * 复制名称到目标缓冲区。
 *
 * @param[in] Hive
 * 指向蜂巢的指针。
 *
 * @param[out] Destination
 * 指向目标缓冲区的指针。
 *
 * @param[in] Source
 * 指向源Unicode字符串结构的指针。
 *
 * @return
 * 返回复制名称的长度。
 */
USHORT
CmpCopyName(IN PHHIVE Hive,
            OUT PWCHAR Destination,
            IN PCUNICODE_STRING Source)
{
    ULONG i;

    /* 检查旧蜂巢 */
    if (Hive->Version == 1)
    {
        /* 直接复制源 */
        RtlCopyMemory(Destination, Source->Buffer, Source->Length);
        return Source->Length;
    }

    /* 对于新版本，检查压缩名称 */
    for (i = 0; i < (Source->Length / sizeof(WCHAR)); i++)
    {
        /* 检查名称是否未压缩 */
        if (Source->Buffer[i] > (unsigned char)-1)
        {
            /* 进行复制 */
            RtlCopyMemory(Destination, Source->Buffer, Source->Length);
            return Source->Length;
        }

        /* 复制这个字符 */
        ((char*)Destination)[i] = (CHAR)(Source->Buffer[i]);
    }

    /* 压缩名称，返回长度 */
    return Source->Length / sizeof(WCHAR);
}

/**
 * @brief 复制压缩名称到目标缓冲区。
 *
 * 该函数将源压缩名称复制到目标缓冲区中，确保不超过目标缓冲区的长度。
 *
 * @param Destination 目标缓冲区，用于存储复制的名称。
 * @param DestinationLength 目标缓冲区的长度（以字节为单位）。
 * @param Source 源压缩名称缓冲区。
 * @param SourceLength 源压缩名称的长度（以字节为单位）。
 */
void
CmpCopyCompressedName(OUT PWCHAR Destination,
                      IN ULONG DestinationLength,
                      IN PWCHAR Source,
                      IN ULONG SourceLength)
{
    ULONG i, Length;

    /* 获取实际需要复制的字符长度 */
    Length = mininum(DestinationLength / sizeof(WCHAR), SourceLength);
    for (i = 0; i < Length; i++)
    {
        /* 复制每个字符 */
        Destination[i] = (WCHAR)((unsigned char*)Source)[i];
    }
}

/**
 * @brief 计算名称的大小。
 *
 * 该函数根据Hive的版本和名称的类型计算名称的大小。
 *
 * @param Hive Hive结构体指针。
 * @param Name 名称的UNICODE字符串结构体指针。
 * @return 名称的长度（以字节为单位）。
 */
USHORT

CmpNameSize(IN PHHIVE Hive,
            IN PCUNICODE_STRING Name)
{
    ULONG i;

    /* 对于旧版本的Hive，直接返回名称的长度 */
    if (Hive->Version == 1) return Name->Length;

    /* 对于新版本的Hive，检查名称是否为压缩名称 */
    for (i = 0; i < (Name->Length / sizeof(WCHAR)); i++)
    {
        /* 检查名称是否为非压缩名称 */
        if (Name->Buffer[i] > (unsigned char)-1) return Name->Length;
    }

    /* 压缩名称，返回长度 */
    return Name->Length / sizeof(WCHAR);
}

/**
 * @brief 计算压缩名称的大小。
 *
 * 该函数计算压缩名称的大小，返回以字节为单位的长度。
 *
 * @param Name 压缩名称缓冲区。
 * @param Length 压缩名称的长度（以字节为单位）。
 * @return 压缩名称的长度（以字节为单位）。
 */
USHORT

CmpCompressedNameSize(IN PWCHAR Name,
                      IN ULONG Length)
{
    /*
     * 不要移除这段注释：压缩名称是“不透明的”，即使当前实现将它们转换为ansi名称，
     * 也不意味着这种情况会永远保持不变，所以不要假设下面的代码内部会这样做！
     */
    return (USHORT)Length * sizeof(WCHAR);
}

/**
 * @brief 比较压缩名称和搜索名称。
 *
 * 该函数比较压缩名称和搜索名称，返回它们之间的差异。
 *
 * @param SearchName 搜索名称的UNICODE字符串结构体指针。
 * @param CompressedName 压缩名称缓冲区。
 * @param NameLength 压缩名称的长度（以字节为单位）。
 * @return 比较结果，如果相等则返回0，否则返回非零值。
 */
LONG

CmpCompareCompressedName(IN PCUNICODE_STRING SearchName,
                         IN PWCHAR CompressedName,
                         IN ULONG NameLength)
{
    WCHAR* p;
    unsigned char* pp;
    WCHAR chr1, chr2;
    USHORT SearchLength;
    LONG Result;

    /* 设置指针和长度，然后循环比较 */
    p = SearchName->Buffer;
    pp = (unsigned char*)CompressedName;
    SearchLength = (SearchName->Length / sizeof(WCHAR));
    while (SearchLength > 0 && NameLength > 0)
    {
        /* 获取字符 */
        chr1 = *p++;
        chr2 = (WCHAR)(*pp++);

        /* 检查是否直接匹配 */
        if (chr1 != chr2)
        {
            /* 检查是否匹配，如果不匹配则返回结果 */
            Result = (LONG)RtlUpcaseUnicodeChar(chr1) -
                     (LONG)RtlUpcaseUnicodeChar(chr2);
            if (Result) return Result;
        }

        /* 下一个字符 */
        SearchLength--;
        NameLength--;
    }

    /* 直接返回差异 */
    return SearchLength - NameLength;
}


/**
 * @brief 在子列表中查找名称。
 *
 * 该函数在给定的子列表中查找指定的名称，并返回找到的子索引和单元格索引。
 *
 * @param Hive Hive结构体指针。
 * @param ChildList 子列表结构体指针。
 * @param Name 要查找的名称的UNICODE字符串结构体指针。
 * @param ChildIndex 可选参数，用于存储找到的子索引。
 * @param CellIndex 用于存储找到的单元格索引。
 * @return 如果找到名称则返回TRUE，否则返回FALSE。
 */
BOOLEAN
CmpFindNameInList(IN PHHIVE Hive,
                  IN PCHILD_LIST ChildList,
                  IN PCUNICODE_STRING Name,
                  OUT PULONG ChildIndex OPTIONAL,
                  OUT PHCELL_INDEX CellIndex)
{
    PCELL_DATA CellData;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    ULONG i;
    PCM_KEY_VALUE KeyValue;
    LONG Result;
    UNICODE_STRING SearchName;
    BOOLEAN Success;

    /* 确保列表中确实有内容 */
    if (ChildList->Count != 0)
    {
        /* 获取单元格数据 */
        CellData = reinterpret_cast<PCELL_DATA>(HvGetCell(Hive, ChildList->List));
        if (!CellData)
        {
            /* 无法获取单元格... 告知调用者 */
            *CellIndex = HCELL_NIL;
            return FALSE;
        }

        /* 现在循环每个条目 */
        for (i = 0; i < ChildList->Count; i++)
        {
            /* 检查是否有需要释放的单元格 */
            if (CellToRelease != HCELL_NIL)
            {
                /* 释放它 */
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }

            /* 获取这个值 */
            KeyValue = reinterpret_cast<PCM_KEY_VALUE>(HvGetCell(Hive, CellData->u.KeyList[i]));
            if (!KeyValue)
            {
                /* 返回未找到数据 */
                *CellIndex = HCELL_NIL;
                Success = FALSE;
                goto Return;
            }

            /* 保存需要释放的单元格 */
            CellToRelease = CellData->u.KeyList[i];

            /* 检查是否为压缩值名称 */
            if (KeyValue->Flags & VALUE_COMP_NAME)
            {
                /* 比较压缩名称 */
                Result = CmpCompareCompressedName(Name,
                                                  KeyValue->Name,
                                                  KeyValue->NameLength);
            }
            else
            {
                /* 直接比较Unicode名称 */
                SearchName.Length = KeyValue->NameLength;
                SearchName.MaximumLength = SearchName.Length;
                SearchName.Buffer = KeyValue->Name;
                Result = RtlCompareUnicodeString(Name, &SearchName, TRUE);
            }

            /* 检查是否找到 */
            if (!Result)
            {
                /* 找到了... 返回信息给调用者 */
                if (ChildIndex) *ChildIndex = i;
                *CellIndex = CellData->u.KeyList[i];

                /* 设置成功状态 */
                Success = TRUE;
                goto Return;
            }
        }

        /* 到达列表末尾 */
        if (ChildIndex) *ChildIndex = i;
        *CellIndex = HCELL_NIL;

        /* 如果到这里表示未找到 */
        Success = TRUE;
        goto Return;
    }

    /* 未找到... 检查调用者是否需要更多信息 */
    ASSERT(ChildList->Count == 0);
    if (ChildIndex) *ChildIndex = 0;
    *CellIndex = HCELL_NIL;

    /* 如果到这里表示未找到 */
    return TRUE;

Return:
    /* 释放第一个获取的单元格 */
    if (CellData) HvReleaseCell(Hive, ChildList->List);

    /* 如果有第二个单元格，释放它 */
    if (CellToRelease) HvReleaseCell(Hive, CellToRelease);
    return Success;
}

