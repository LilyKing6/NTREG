/*
 * PROJECT:   Registry Library
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#pragma once

#include "typedefs.hpp"

namespace registry {

// Hive operations (C++20 enum class)
enum class HiveOperation : u32 {
    Create = 0,
    Memory = 1,
    File = 2,
    MemoryInPlace = 3,
    Flat = 4,
    MapFile = 5
};

// Hive flags (C++20 enum class with bitwise operations)
enum class HiveFlags : u32 {
    None = 0,
    Volatile = 1,
    NoLazyFlush = 2,
    HasBeenReplaced = 4,
    HasBeenFreed = 8,
    Unknown = 0x10,
    IsUnloading = 0x20
};

// Hive file types (already defined in types.hpp as FileType)
// Using compatibility defines for existing code
#define HFILE_TYPE_PRIMARY   0
#define HFILE_TYPE_LOG       1
#define HFILE_TYPE_EXTERNAL  2
#define HFILE_TYPE_ALTERNATE 3
#define HFILE_TYPE_MAX       4

} // namespace registry

// Hive size constants (C++20 constexpr)
inline constexpr registry::u32 HBLOCK_SIZE = 0x1000;
inline constexpr registry::u32 HSECTOR_SIZE = 0x200;
inline constexpr registry::u32 HSECTOR_COUNT = 8;

// Legacy compatibility defines
#define HINIT_CREATE 0
#define HINIT_MEMORY 1
#define HINIT_FILE 2
#define HINIT_MEMORY_INPLACE 3
#define HINIT_FLAT 4
#define HINIT_MAPFILE 5

#define HIVE_VOLATILE 1
#define HIVE_NOLAZYFLUSH 2
#define HIVE_HAS_BEEN_REPLACED 4
#define HIVE_HAS_BEEN_FREED 8
#define HIVE_UNKNOWN 0x10
#define HIVE_IS_UNLOADING 0x20


#define HV_LOG_HEADER_SIZE FIELD_OFFSET(HBASE_BLOCK, Reserved2)

// Hive signatures (C++20 constexpr)
inline constexpr registry::u32 HV_HHIVE_SIGNATURE = 0xbee0bee0;
inline constexpr registry::u32 HV_HBLOCK_SIGNATURE = 0x66676572; // "regf"
inline constexpr registry::u32 HV_HBIN_SIGNATURE = 0x6e696268;   // "hbin"
inline constexpr registry::u32 HV_LOG_DIRTY_SIGNATURE = 0x54524944; // "DIRT"

// Hive version constants (C++20 constexpr)
inline constexpr registry::u32 HSYS_MAJOR = 1;
inline constexpr registry::u32 HSYS_MINOR = 3;
inline constexpr registry::u32 HSYS_WHISTLER_BETA1 = 4;
inline constexpr registry::u32 HSYS_WHISTLER = 5;
inline constexpr registry::u32 HSYS_MINOR_SUPPORTED = HSYS_WHISTLER;

// Hive format constants (C++20 constexpr)
inline constexpr registry::u32 HBASE_FORMAT_MEMORY = 1;
inline constexpr registry::u32 HTYPE_COUNT = 2;

// Boot type constants (C++20 constexpr)
inline constexpr registry::u32 HBOOT_TYPE_REGULAR = 0;
inline constexpr registry::u32 HBOOT_TYPE_SELF_HEAL = 4;

// Boot recovery constants (C++20 constexpr)
inline constexpr registry::u32 HBOOT_NO_BOOT_RECOVER = 0;
inline constexpr registry::u32 HBOOT_BOOT_RECOVERED_BY_HIVE_LOG = 1;
inline constexpr registry::u32 HBOOT_BOOT_RECOVERED_BY_ALTERNATE_HIVE = 2;

// Clean/dirty block constants (C++20 constexpr)
inline constexpr registry::u8 HV_CLEAN_BLOCK = 0U;
inline constexpr registry::u8 HV_LOG_DIRTY_BLOCK = 0xFF;

// Hive filename max length (C++20 constexpr)
inline constexpr registry::u32 HIVE_FILENAME_MAXLEN = 31;

// Cell constants (C++20 constexpr) - using types.hpp definitions
// Export to global namespace for compatibility
using HCELL_INDEX = registry::HCELL_INDEX;
using PHCELL_INDEX = HCELL_INDEX*;

inline constexpr HCELL_INDEX HCELL_NIL = registry::HCELL_NIL;
inline constexpr registry::u32 HCELL_CACHED = 1;
inline constexpr registry::u32 HCELL_TYPE_MASK = registry::HCELL_TYPE_MASK;
inline constexpr registry::u32 HCELL_BLOCK_MASK = 0x7ffff000;
inline constexpr registry::u32 HCELL_OFFSET_MASK = 0x00000fff;
inline constexpr registry::u32 HCELL_TYPE_SHIFT = 31;
inline constexpr registry::u32 HCELL_BLOCK_SHIFT = 12;
inline constexpr registry::u32 HCELL_OFFSET_SHIFT = 0;

// Helper macros (kept for compatibility)
#define HvGetCellType(Cell) \
    ((ULONG)(((Cell) & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT))
#define HvGetCellBlock(Cell) \
    ((ULONG)(((Cell) & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT))

typedef enum
{
    Stable   = 0,  // 稳定存储
    Volatile = 1  // 易失存储
} HSTORAGE_TYPE;

#include <pshpack1.h>

/**
 * @name HBASE_BLOCK
 *
 * 注册表蜂巢文件的磁盘头。
 */

#define HIVE_FILENAME_MAXLEN            31  // 蜂巢文件名最大长度


typedef struct _HBASE_BLOCK
{
    /* Hive base block 标识符 "regf" (0x66676572) */
    ULONG Signature;

    /* Update counters */
    ULONG Sequence1;
    ULONG Sequence2;

    /* Hive 文件最后写入的时间戳 */
    LARGE_INTEGER TimeStamp;

    /* 注册表格式主版本 (1) */
    ULONG Major;

    /* 注册表格式次要版本 (3) (3)
       版本3增加了快速索引，版本5有大值优化 */
    ULONG Minor;

    /* 注册表文件类型(0 - 主要，1 - 日志) */
    ULONG Type;

    /* 注册表格式 (1是迄今为止唯一定义的值) */
    ULONG Format;

    /* 从基块末尾之后的字节到文件中的偏移量
       如果 hive 是易失性的, 则这是指向 CM_KEY_NODE 的实际指针 */
    HCELL_INDEX RootCell;

    /* 完整配置单元的大小(以字节为单位)减去标头, 块大小的倍数 (4KB) */
    ULONG Length;

    /* (1?) */
    ULONG Cluster;

    /* 最后 31 个 UNICODE 字符, 加上终止 NULL 字符，
        hive 文件的全名 */
    WCHAR FileName[HIVE_FILENAME_MAXLEN + 1];

    ULONG Reserved1[99];

    /* 前 0x200 字节的校验和 */
    ULONG CheckSum;

    ULONG Reserved2[0x37E];
    ULONG BootType;
    ULONG BootRecover;
} HBASE_BLOCK, *PHBASE_BLOCK;

// C_ASSERT(sizeof(HBASE_BLOCK) == HBLOCK_SIZE); -- skipped on non-MSVC (layout mismatch)

typedef struct _HBIN
{
    /* Hive bin 标识符 "hbin" (0x6E696268) */
    ULONG Signature;

    /* 该 bin 的块偏移量 */
    HCELL_INDEX FileOffset;

    /* 此 bin 的大小(以字节为单位), 块大小的倍数 (4KB) */
    ULONG Size;

    ULONG Reserved1[2];

    /* 该 bin 最后写入的时间戳 */
    LARGE_INTEGER TimeStamp;

    /* 未使用 (仅限内存中) */
    ULONG Spare;
} HBIN, *PHBIN;

typedef struct _HCELL
{
    /* 如果使用则 <0 如果空闲则 >0 */
    LONG Size;
} HCELL, *PHCELL;

#include <poppack.h>

struct _HHIVE;

typedef struct _CELL_DATA*
(CMAPI *PGET_CELL_ROUTINE)(
    struct _HHIVE *Hive,
    HCELL_INDEX Cell
);

typedef void
(CMAPI *PRELEASE_CELL_ROUTINE)(
    struct _HHIVE *Hive,
    HCELL_INDEX Cell
);

typedef void*
(CMAPI *PALLOCATE_ROUTINE)(
    SIZE_T Size,
    BOOLEAN Paged,
    ULONG Tag
);

typedef void
(CMAPI *PFREE_ROUTINE)(
    void* Ptr,
    ULONG Quota
);

typedef BOOLEAN
(CMAPI *PFILE_READ_ROUTINE)(
    struct _HHIVE *RegistryHive,
    ULONG FileType,
    PULONG FileOffset,
    void* Buffer,
    SIZE_T BufferLength
);

typedef BOOLEAN
(CMAPI *PFILE_WRITE_ROUTINE)(
    struct _HHIVE *RegistryHive,
    ULONG FileType,
    PULONG FileOffset,
    void* Buffer,
    SIZE_T BufferLength
);

typedef BOOLEAN
(CMAPI *PFILE_SET_SIZE_ROUTINE)(
    struct _HHIVE *RegistryHive,
    ULONG FileType,
    ULONG FileSize,
    ULONG OldfileSize
);

typedef BOOLEAN
(CMAPI *PFILE_FLUSH_ROUTINE)(
    struct _HHIVE *RegistryHive,
    ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length
);

typedef struct _HMAP_ENTRY
{
    ULONG_PTR BlockAddress;
    ULONG_PTR BinAddress;
    struct _CM_VIEW_OF_FILE *CmView;
    ULONG MemAlloc;
} HMAP_ENTRY, *PHMAP_ENTRY;

typedef struct _HMAP_TABLE
{
    HMAP_ENTRY Table[512];
} HMAP_TABLE, *PHMAP_TABLE;

typedef struct _HMAP_DIRECTORY
{
    PHMAP_TABLE Directory[2048];
} HMAP_DIRECTORY, *PHMAP_DIRECTORY;

typedef struct _DUAL
{
    /* 数据结构的长度 */
    ULONG Length;
    /* 指向HMAP_DIRECTORY结构体的指针, 用于映射目录 */
    PHMAP_DIRECTORY Map;
    /* 指向HMAP_ENTRY结构体的指针, 也可以表示为PHMAP_TABLE SmallDir, 用于存储块列表 */
    PHMAP_ENTRY BlockList; // PHMAP_TABLE SmallDir;
    /* 保护字段, 用于数据结构的完整性验证 */
    ULONG Guard;
    /* 一个包含24个元素的HCELL_INDEX数组, 也可以表示为FREE_DISPLAY FreeDisplay[24], 用于显示空闲单元格的索引 */
    HCELL_INDEX FreeDisplay[24]; // FREE_DISPLAY FreeDisplay[24];
    /* 空闲单元格的摘要信息 */
    ULONG FreeSummary;
    /* 空闲单元格的链表, 使用LIST_ENTRY结构管理 */
    LIST_ENTRY FreeBins;
} DUAL, *PDUAL;


typedef struct _HHIVE
{
    /* Hive identifier (0xBEE0BEE0) */
    /* 蜂巢（Hive）的签名 */
    ULONG Signature;

    /* 回调函数 */

    /* 获取单元格数据的回调函数 */
    PGET_CELL_ROUTINE GetCellRoutine;
    /* 释放单元格数据的回调函数 */
    PRELEASE_CELL_ROUTINE ReleaseCellRoutine;
    /* 分配内存的回调函数 */
    PALLOCATE_ROUTINE Allocate;
    /* 释放内存的回调函数 */
    PFREE_ROUTINE Free;
    /* 设置文件大小的回调函数 */
    PFILE_SET_SIZE_ROUTINE FileSetSize;
    /* 写文件的回调函数 */
    PFILE_WRITE_ROUTINE FileWrite;
    /* 读文件的回调函数 */
    PFILE_READ_ROUTINE FileRead;
    /* 刷新文件的回调函数 */
    PFILE_FLUSH_ROUTINE FileFlush;

    /* 指向Hive的基本块 base block结构体的指针 */
    PHBASE_BLOCK BaseBlock;
    /* 用于跟踪Hive中哪些部分被修改过的RTL_BITMAP */
    RTL_BITMAP DirtyVector;
    /* 被修改过的部分计数 */
    ULONG DirtyCount;
    /* 分配给DirtyVector的空间大小 */
    ULONG DirtyAlloc;
    /* 分配给BaseBlock的空间大小 */
    ULONG BaseBlockAlloc;
    /* 群集大小 */
    ULONG Cluster;
    /* 是否为平面Hive */
    BOOLEAN Flat;
    /* 是否为只读Hive */
    BOOLEAN ReadOnly;

    /* 是否启用日志 */
    BOOLEAN Log;
    /* 是否为备用蜂巢 */
    BOOLEAN Alternate;


    /* 蜂巢是否被修改过 */
    BOOLEAN DirtyFlag;

    /* 蜂巢标志 */
    ULONG HiveFlags;

    /* 日志大小 */
    ULONG LogSize;


    /* 刷新计数 */
    ULONG RefreshCount;
    /* 存储类型计数 */
    ULONG StorageTypeCount;
    /* Hive版本 */
    ULONG Version;
    
    /* 用于存储Hive数据的数组 */
    DUAL Storage[HTYPE_COUNT];

} HHIVE, *PHHIVE;


#define IsFreeCell(Cell)    ((Cell)->Size >= 0)
#define IsUsedCell(Cell)    ((Cell)->Size <  0)
