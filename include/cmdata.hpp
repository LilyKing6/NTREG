/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#ifndef _CMDATA_H_
#define _CMDATA_H_

#include "typedefs.hpp"

// Key index types (C++20 constexpr)
inline constexpr registry::u16 CM_KEY_INDEX_ROOT = 0x6972;  // "ri"
inline constexpr registry::u16 CM_KEY_INDEX_LEAF = 0x696C;  // "li"
inline constexpr registry::u16 CM_KEY_FAST_LEAF = 0x666C;   // "lf"
inline constexpr registry::u16 CM_KEY_HASH_LEAF = 0x686C;   // "lh"

// Key signatures (C++20 constexpr)
inline constexpr registry::u16 CM_KEY_NODE_SIGNATURE = 0x6B6E;      // "nk"
inline constexpr registry::u16 CM_LINK_NODE_SIGNATURE = 0x6B6C;     // "lk"
inline constexpr registry::u16 CM_KEY_SECURITY_SIGNATURE = 0x6B73;  // "sk"
inline constexpr registry::u16 CM_KEY_VALUE_SIGNATURE = 0x6B76;     // "vk"
inline constexpr registry::u16 CM_BIG_DATA_SIGNATURE = 0x6264;      // "db"

// Key access rights (C++20 constexpr)
#undef KEY_WRITE
#undef KEY_EXECUTE
#undef KEY_READ
#undef KEY_ALL_ACCESS
inline constexpr registry::u32 KEY_WRITE = 0x20006;
inline constexpr registry::u32 KEY_EXECUTE = 0x20019;
inline constexpr registry::u32 KEY_READ = 0x20019;
inline constexpr registry::u32 KEY_ALL_ACCESS = 0xf003f;

// CM_KEY_NODE flags (C++20 constexpr)
inline constexpr registry::u16 KEY_IS_VOLATILE = 0x0001;
inline constexpr registry::u16 KEY_HIVE_EXIT = 0x0002;
inline constexpr registry::u16 KEY_HIVE_ENTRY = 0x0004;
inline constexpr registry::u16 KEY_NO_DELETE = 0x0008;
inline constexpr registry::u16 KEY_SYM_LINK = 0x0010;
inline constexpr registry::u16 KEY_COMP_NAME = 0x0020;
inline constexpr registry::u16 KEY_PREDEF_HANDLE = 0x0040;
inline constexpr registry::u16 KEY_VIRT_MIRRORED = 0x0080;
inline constexpr registry::u16 KEY_VIRT_TARGET = 0x0100;
inline constexpr registry::u16 KEY_VIRTUAL_STORE = 0x0200;

// CM_KEY_VALUE flags (C++20 constexpr)
inline constexpr registry::u16 VALUE_COMP_NAME = 0x0001;

// CM_KEY_VALUE size constants (C++20 constexpr)
inline constexpr registry::u32 CM_KEY_VALUE_SMALL = 0x4;
inline constexpr registry::u32 CM_KEY_VALUE_BIG = 0x3FD8;
inline constexpr registry::u32 CM_KEY_VALUE_SPECIAL_SIZE = 0x80000000;

#include <pshpack1.h>


//
// 内存映射蜂巢的视图
//
typedef struct _CM_VIEW_OF_FILE
{
    LIST_ENTRY LRUViewList;  // LRU视图列表
    LIST_ENTRY PinViewList;  // 固定视图列表
    ULONG FileOffset;  // 文件偏移量
    ULONG Size;  // 大小
    PULONG_PTR ViewAddress;  // 视图地址
    void* Bcb;  // Bcb指针
    ULONG UseCount;  // 使用计数
} CM_VIEW_OF_FILE, *PCM_VIEW_OF_FILE;

//
// 键节点的子节点
//
typedef struct _CHILD_LIST
{
    ULONG Count;  // 子节点数量
    HCELL_INDEX List;  // 子节点列表
} CHILD_LIST, *PCHILD_LIST;

//
// 键节点对父节点的引用
//
typedef struct  _CM_KEY_REFERENCE
{
    HCELL_INDEX KeyCell;  // 键单元格索引
    PHHIVE KeyHive;  // 键蜂巢指针
} CM_KEY_REFERENCE, *PCM_KEY_REFERENCE;

//
// 键节点
//
typedef struct _CM_KEY_NODE
{
    USHORT Signature;  // 签名
    USHORT Flags;  // 标志
    LARGE_INTEGER LastWriteTime;  // 最后写入时间
    ULONG Spare;  // 备用字段
    HCELL_INDEX Parent;  // 父节点索引
    ULONG SubKeyCounts[HTYPE_COUNT];  // 子键计数
    union
    {
        struct
        {
            HCELL_INDEX SubKeyLists[HTYPE_COUNT];  // 子键列表
            CHILD_LIST ValueList;  // 值列表
        };
        CM_KEY_REFERENCE ChildHiveReference;  // 子蜂巢引用
    };
    HCELL_INDEX Security;  // 安全索引
    HCELL_INDEX Class;  // 类索引
    ULONG MaxNameLen;  // 最大名称长度
    ULONG MaxClassLen;  // 最大类长度
    ULONG MaxValueNameLen;  // 最大值名称长度
    ULONG MaxValueDataLen;  // 最大值数据长度
    ULONG WorkVar;  // 工作变量
    USHORT NameLength;  // 名称长度
    USHORT ClassLength;  // 类长度
    WCHAR Name[ANYSIZE_ARRAY];  // 名称
} CM_KEY_NODE, *PCM_KEY_NODE;

//
// 值键
//
typedef struct _CM_KEY_VALUE
{
    USHORT Signature;  // 签名
    USHORT NameLength;  // 名称长度
    ULONG DataLength;  // 数据长度
    HCELL_INDEX Data;  // 数据索引
    ULONG Type;  // 类型
    USHORT Flags;  // 标志
    USHORT Spare;  // 备用字段
    WCHAR Name[ANYSIZE_ARRAY];  // 名称
} CM_KEY_VALUE, *PCM_KEY_VALUE;

//
// 安全键
//
typedef struct _CM_KEY_SECURITY
{
    USHORT Signature;  // 签名
    USHORT Reserved;  // 保留字段
    HCELL_INDEX Flink;  // 前向链接
    HCELL_INDEX Blink;  // 后向链接
    ULONG ReferenceCount;  // 引用计数
    ULONG DescriptorLength;  // 描述符长度
    SECU_DESC_RELATIVE Descriptor;  // 安全描述符
} CM_KEY_SECURITY, *PCM_KEY_SECURITY;

//
// 大值键
//
typedef struct _CM_BIG_DATA
{
    USHORT Signature;  // 签名
    USHORT Count;  // 计数
    HCELL_INDEX List;  // 列表索引
} CM_BIG_DATA, *PCM_BIG_DATA;

#include <poppack.h>

//
// 通用索引条目
//
typedef struct _CM_INDEX
{
    HCELL_INDEX Cell;  // 单元格索引
    union
    {
        unsigned char NameHint[4];  // 名称提示
        ULONG HashKey;  // 哈希键
    };
} CM_INDEX, *PCM_INDEX;

//
// 键索引
//
typedef struct _CM_KEY_INDEX
{
    USHORT Signature;  // 签名
    USHORT Count;  // 计数
    HCELL_INDEX List[ANYSIZE_ARRAY];  // 列表
} CM_KEY_INDEX, *PCM_KEY_INDEX;

//
// 快速/哈希键索引
//
typedef struct _CM_KEY_FAST_INDEX
{
    USHORT Signature;  // 签名
    USHORT Count;  // 计数
    CM_INDEX List[ANYSIZE_ARRAY];  // 列表
} CM_KEY_FAST_INDEX, *PCM_KEY_FAST_INDEX;

//
// 单元格数据
//
typedef struct _CELL_DATA
{
    union
    {
        CM_KEY_NODE KeyNode;  // 键节点
        CM_KEY_VALUE KeyValue;  // 值键
        CM_KEY_SECURITY KeySecurity;  // 安全键
        CM_KEY_INDEX KeyIndex;  // 键索引
        CM_BIG_DATA ValueData;  // 大值数据
        HCELL_INDEX KeyList[ANYSIZE_ARRAY];  // 键列表
        WCHAR KeyString[ANYSIZE_ARRAY];  // 键字符串
    } u;
} CELL_DATA, *PCELL_DATA;

#endif // _CMDATA_H_
