/*
 * PROJECT:   NTREG
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#ifndef _CMLIB_H_
#define _CMLIB_H_


#include "typedefs.hpp"

#include "unicode.hpp"
#include <stdio.h>
#include <string.h>

/* C_ASSERT Definition */
#undef C_ASSERT
#define C_ASSERT(expr) static_assert((expr), "C_ASSERT: " #expr)

#ifdef _WIN32
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif // _WIN32

#if (!defined(_MSC_VER) || (_MSC_VER < 1500))
#define _In_
#define _Out_
#define _Inout_
#define _In_opt_
#define _In_range_(x, y)
#endif

#define __drv_aliasesMem

#ifndef mininum
#define mininum(a, b)  (((a) < (b)) ? (a) : (b))
#endif

// #ifndef max
// #define max(a, b)  (((a) > (b)) ? (a) : (b))
// #endif

// Definitions copied from <ntstatus.h>
// We only want to include host headers, so we define them manually
#undef STATUS_SUCCESS
#undef STATUS_NOT_IMPLEMENTED
#undef STATUS_NO_MEMORY
#undef STATUS_INSUFFICIENT_RESOURCES
#undef STATUS_INVALID_PARAMETER
#undef STATUS_REGISTRY_CORRUPT
#undef STATUS_REGISTRY_IO_FAILED
#undef STATUS_NOT_REGISTRY_FILE
#undef STATUS_REGISTRY_RECOVERED
#define STATUS_SUCCESS                   ((int)0x00000000) //0
#define STATUS_NOT_IMPLEMENTED           ((int)0xC0000002) //3221225474
#define STATUS_NO_MEMORY                 ((int)0xC0000017) //3221225495
#define STATUS_INSUFFICIENT_RESOURCES    ((int)0xC000009A) //3221225626
#define STATUS_INVALID_PARAMETER         ((int)0xC000000D) //3221225485
#define STATUS_REGISTRY_CORRUPT          ((int)0xC000014C) //3221225804
#define STATUS_REGISTRY_IO_FAILED        ((int)0xC000014D) //3221225805
#define STATUS_NOT_REGISTRY_FILE         ((int)0xC000015C) //3221225820
#define STATUS_REGISTRY_RECOVERED        ((int)0x40000009) //1073741833

/* 默认值, 信息被保存到文件中 */
#undef REG_OPTION_NON_VOLATILE
#define REG_OPTION_NON_VOLATILE          0

/* 易失性子键, 被保存在内存中 */
#undef REG_OPTION_VOLATILE
#define REG_OPTION_VOLATILE              1

#define OBJ_CASE_INSENSITIVE             0x00000040L
#define USHORT_MAX                       USHRT_MAX

#define OBJ_NAME_PATH_SEPARATOR          ((WCHAR)u'\\')
#define UNICODE_NULL                     ((WCHAR)0)

void 
RtlInitUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString);

LONG 
RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive);

// FIXME: DECLSPEC_NORETURN
void
KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4);

void 
KeQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime);

WCHAR 
RtlUpcaseUnicodeChar(
    IN WCHAR Source);

void 
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap);

ULONG 
RtlFindSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex);

void 
RtlSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet);

void 
RtlSetAllBits(
    IN PRTL_BITMAP BitMapHeader);

void 
RtlClearAllBits(
    IN PRTL_BITMAP BitMapHeader);

#undef RtlCheckBit
#undef UNREFERENCED_PARAMETER
#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP) / 32]) >> ((BP) % 32)) & 0x1)
#define UNREFERENCED_PARAMETER(P) ((void)(P))

#define PKTHREAD void*
#define PKGUARDED_MUTEX void*
#define PERESOURCE void*
#define PFILE_OBJECT void*
#define PKEVENT void*
#define PWORK_QUEUE_ITEM void*
#define EX_PUSH_LOCK PULONG_PTR

// Definitions copied from <ntifs.h>
// We only want to include host headers, so we define them manually

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

typedef struct _SECU_DESC_RELATIVE
{
    unsigned char Revision;
    unsigned char Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    ULONG Owner;
    ULONG Group;
    ULONG Sacl;
    ULONG Dacl;
} SECU_DESC_RELATIVE, *PISECU_DESC_RELATIVE;

#define CMLTRACE(x, ...)
#undef PAGED_CODE
#define PAGED_CODE()
#define REGISTRY_ERROR                   ((ULONG)0x00000051L)



//
// These define the Debug Masks Supported
//
#define CMLIB_HCELL_DEBUG       0x01

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

//
// PAGE_SIZE definition
//
#ifndef PAGE_SIZE

    #define PAGE_SIZE 0x1000

#endif

#define TAG_CM             '  MC'
#define TAG_KCB            'bkMC'
#define TAG_CMHIVE         'vHMC'
#define TAG_CMSD           'DSMC'
#define TAG_REGISTRY_STACK 'sRMC'

#undef CMAPI
#define CMAPI 

//
// Check Registry status type definition
//
typedef ULONG CM_CHECK_REGISTRY_STATUS;

//
// Check Registry flags
//
#define CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES        0x0
#define CM_CHECK_REGISTRY_PURGE_VOLATILES             0x2
#define CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES  0x4
#define CM_CHECK_REGISTRY_VALIDATE_HIVE               0x8
#define CM_CHECK_REGISTRY_FIX_HIVE                    0x10

//
// Check Registry status codes
//
#define CM_CHECK_REGISTRY_GOOD                         0
#define CM_CHECK_REGISTRY_INVALID_PARAMETER            1
#define CM_CHECK_REGISTRY_SD_INVALID                   2
#define CM_CHECK_REGISTRY_HIVE_CORRUPT_SIGNATURE       3
#define CM_CHECK_REGISTRY_BIN_SIZE_OR_OFFSET_CORRUPT   4
#define CM_CHECK_REGISTRY_BIN_SIGNATURE_HEADER_CORRUPT 5
#define CM_CHECK_REGISTRY_BAD_FREE_CELL                6
#define CM_CHECK_REGISTRY_BAD_ALLOC_CELL               7
#define CM_CHECK_REGISTRY_ALLOCATE_MEM_STACK_FAIL      8
#define CM_CHECK_REGISTRY_ROOT_CELL_NOT_FOUND          9
#define CM_CHECK_REGISTRY_BAD_LEXICOGRAPHICAL_ORDER    10
#define CM_CHECK_REGISTRY_NODE_NOT_FOUND               11
#define CM_CHECK_REGISTRY_SUBKEY_NOT_FOUND             12
#define CM_CHECK_REGISTRY_TREE_TOO_MANY_LEVELS         13
#define CM_CHECK_REGISTRY_KEY_CELL_NOT_ALLOCATED       14
#define CM_CHECK_REGISTRY_CELL_DATA_NOT_FOUND          15
#define CM_CHECK_REGISTRY_CELL_SIZE_NOT_SANE           16
#define CM_CHECK_REGISTRY_KEY_NAME_LENGTH_ZERO         17
#define CM_CHECK_REGISTRY_KEY_TOO_BIG_THAN_CELL        18
#define CM_CHECK_REGISTRY_BAD_KEY_NODE_PARENT          19
#define CM_CHECK_REGISTRY_BAD_KEY_NODE_SIGNATURE       20
#define CM_CHECK_REGISTRY_KEY_CLASS_UNALLOCATED        21
#define CM_CHECK_REGISTRY_VALUE_LIST_UNALLOCATED       22
#define CM_CHECK_REGISTRY_VALUE_LIST_DATA_NOT_FOUND    23
#define CM_CHECK_REGISTRY_VALUE_LIST_SIZE_NOT_SANE     24
#define CM_CHECK_REGISTRY_VALUE_CELL_NIL               25
#define CM_CHECK_REGISTRY_VALUE_CELL_UNALLOCATED       26
#define CM_CHECK_REGISTRY_VALUE_CELL_DATA_NOT_FOUND    27
#define CM_CHECK_REGISTRY_VALUE_CELL_SIZE_NOT_SANE     28
#define CM_CHECK_REGISTRY_CORRUPT_VALUE_DATA           29
#define CM_CHECK_REGISTRY_DATA_CELL_NOT_ALLOCATED      30
#define CM_CHECK_REGISTRY_BAD_KEY_VALUE_SIGNATURE      31
#define CM_CHECK_REGISTRY_STABLE_KEYS_ON_VOLATILE      32
#define CM_CHECK_REGISTRY_SUBKEYS_LIST_UNALLOCATED     33
#define CM_CHECK_REGISTRY_CORRUPT_SUBKEYS_INDEX        34
#define CM_CHECK_REGISTRY_BAD_SUBKEY_COUNT             35
#define CM_CHECK_REGISTRY_KEY_INDEX_CELL_UNALLOCATED   36
#define CM_CHECK_REGISTRY_CORRUPT_LEAF_ON_ROOT         37
#define CM_CHECK_REGISTRY_CORRUPT_LEAF_SIGNATURE       38
#define CM_CHECK_REGISTRY_CORRUPT_KEY_INDEX_SIGNATURE  39

//
// Check Registry success macro
//
#define CM_CHECK_REGISTRY_SUCCESS(StatusCode) ((ULONG)(StatusCode) == CM_CHECK_REGISTRY_GOOD)

// #include <unicode.h>
#include <wchar.h>
#include "hivedata.hpp"
#include "cmdata.hpp"

/* 前向声明 */
typedef struct _CM_KEY_SECURITY_CACHE_ENTRY *PCM_KEY_SECURITY_CACHE_ENTRY; // 定义CM_KEY_SECURITY_CACHE_ENTRY结构体的指针类型
typedef struct _CM_KEY_CONTROL_BLOCK *PCM_KEY_CONTROL_BLOCK; // 定义CM_KEY_CONTROL_BLOCK结构体的指针类型
typedef struct _CM_CELL_REMAP_BLOCK *PCM_CELL_REMAP_BLOCK; // 定义CM_CELL_REMAP_BLOCK结构体的指针类型

// 参见ntoskrnl/include/internal/cm.h
#define CMP_SECURITY_HASH_LISTS     64 // 定义安全哈希列表的数量

///////////////////////////////////////////////////////////////

typedef struct _FILE_STANDARD_INFORMATION
{
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

//
// I/O Status Block
//
typedef struct _IO_STATUS_BLOCK
{
    union
    {
        int Status;
        void* Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

////////////////////////////////////////////////////////////

//
// 使用计数日志和条目
//
typedef struct _CM_USE_COUNT_LOG_ENTRY
{
    HCELL_INDEX Cell; // 单元格索引
    void* Stack[7]; // 堆栈信息
} CM_USE_COUNT_LOG_ENTRY, *PCM_USE_COUNT_LOG_ENTRY; // 定义使用计数日志条目结构体及其指针类型

typedef struct _CM_USE_COUNT_LOG
{
    USHORT Next; // 下一个条目的索引
    USHORT Size; // 日志的大小
    CM_USE_COUNT_LOG_ENTRY Log[32]; // 日志条目数组
} CM_USE_COUNT_LOG, *PCM_USE_COUNT_LOG; // 定义使用计数日志结构体及其指针类型

//
// 配置管理器蜂巢结构
//
typedef struct _CMHIVE
{
    HHIVE Hive; // 蜂巢结构
    HANDLE FileHandles[HFILE_TYPE_MAX]; // 文件句柄数组
    
    LIST_ENTRY NotifyList; // 通知列表
    LIST_ENTRY HiveList; // 蜂巢列表

    EX_PUSH_LOCK HiveLock; // 蜂巢锁
    PKTHREAD HiveLockOwner; // 蜂巢锁所有者
    PKGUARDED_MUTEX ViewLock; // 视图锁
    PKTHREAD ViewLockOwner; // 视图锁所有者
    EX_PUSH_LOCK WriterLock; // 写者锁
    PKTHREAD WriterLockOwner; // 写者锁所有者
    PERESOURCE FlusherLock; // 刷新锁
    EX_PUSH_LOCK SecurityLock; // 安全锁
    PKTHREAD HiveSecurityLockOwner; // 蜂巢安全锁所有者
    LIST_ENTRY LRUViewListHead; // LRU视图列表头
    LIST_ENTRY PinViewListHead; // 固定视图列表头
    PFILE_OBJECT FileObject; // 文件对象
    UNICODE_STRING FileFullPath; // 文件全路径
    UNICODE_STRING FileUserName; // 文件用户名
    USHORT MappedViews; // 映射视图数量
    USHORT PinnedViews; // 固定视图数量
    ULONG UseCount; // 使用计数
    ULONG SecurityCount; // 安全计数
    ULONG SecurityCacheSize; // 安全缓存大小
    LONG SecurityHitHint; // 安全命中提示
    PCM_KEY_SECURITY_CACHE_ENTRY SecurityCache; // 安全缓存
    LIST_ENTRY SecurityHash[CMP_SECURITY_HASH_LISTS]; // 安全哈希列表
    PKEVENT UnloadEvent; // 卸载事件
    PCM_KEY_CONTROL_BLOCK RootKcb; // 根KCB
    BOOLEAN Frozen; // 是否冻结
    PWORK_QUEUE_ITEM UnloadWorkItem; // 卸载工作项
    BOOLEAN GrowOnlyMode; // 仅增长模式
    ULONG GrowOffset; // 增长偏移量
    LIST_ENTRY KcbConvertListHead; // KCB转换列表头
    LIST_ENTRY KnodeConvertListHead; // Knode转换列表头
    PCM_CELL_REMAP_BLOCK CellRemapArray; // 单元格重映射数组
    CM_USE_COUNT_LOG UseCountLog; // 使用计数日志
    CM_USE_COUNT_LOG LockHiveLog; // 锁定蜂巢日志
    ULONG Flags; // 标志
    LIST_ENTRY TrustClassEntry; // 信任类条目
    ULONG FlushCount; // 刷新计数
    BOOLEAN HiveIsLoading; // 蜂巢是否正在加载
    PKTHREAD CreatorOwner; // 创建者所有者
} CMHIVE, *PCMHIVE; // 定义蜂巢结构体及其指针类型

typedef struct _HV_HIVE_CELL_PAIR
{
    PHHIVE Hive; // 蜂巢指针
    HCELL_INDEX Cell; // 单元格索引
} HV_HIVE_CELL_PAIR, *PHV_HIVE_CELL_PAIR; // 定义蜂巢单元格对结构体及其指针类型

#define STATIC_CELL_PAIR_COUNT 4 // 静态单元格对数量
typedef struct _HV_TRACK_CELL_REF
{
    USHORT Count; // 计数
    USHORT Max; // 最大值
    PHV_HIVE_CELL_PAIR CellArray; // 单元格数组
    HV_HIVE_CELL_PAIR StaticArray[STATIC_CELL_PAIR_COUNT]; // 静态数组
    USHORT StaticCount; // 静态计数
} HV_TRACK_CELL_REF, *PHV_TRACK_CELL_REF; // 定义跟踪单元格引用结构体及其指针类型

extern ULONG CmlibTraceLevel; // 外部声明Cmlib跟踪级别

//
// 由于大键尚未支持，这里是一个临时的解决方案
//
#ifdef _BLDR_ 
#define ASSERT_VALUE_BIG(h, s) \
    do { if (CmpIsKeyValueBig(h,s)) DPRINT("Big keys aren't supported!\n"); } while (0)
#else 
#define ASSERT_VALUE_BIG(h, s)  \
    ASSERTMSG("Big keys not supported!\n", !CmpIsKeyValueBig(h, s));
#endif


//
// Returns whether or not this is a small valued key
//
static inline
BOOLEAN
CmpIsKeyValueSmall(OUT PULONG RealLength,
                   IN ULONG Length)
{
    /* Check if the length has the special size value */
    if (Length >= CM_KEY_VALUE_SPECIAL_SIZE)
    {
        /* It does, so this is a small key: return the real length */
        *RealLength = Length - CM_KEY_VALUE_SPECIAL_SIZE;
        return TRUE;
    }

    /* This is not a small key, return the length we read */
    *RealLength = Length;
    return FALSE;
}

//
// Returns whether or not this is a big valued key
//
static inline
BOOLEAN
CmpIsKeyValueBig(IN PHHIVE Hive,
                 IN ULONG Length)
{
    /* Check if the hive is XP Beta 1 or newer */
    if (Hive->Version >= HSYS_WHISTLER_BETA1)
    {
        /* Check if the key length is valid for a big value key */
        if ((Length < CM_KEY_VALUE_SPECIAL_SIZE) && (Length > CM_KEY_VALUE_BIG))
        {
            /* Yes, this value is big */
            return TRUE;
        }
    }

    /* Not a big value key */
    return FALSE;
}

/*
 * Public Hive functions.
 */
int CMAPI
HvInitialize(
    PHHIVE RegistryHive,
    ULONG OperationType,
    ULONG HiveFlags,
    ULONG FileType,
    void* HiveData OPTIONAL,
    PALLOCATE_ROUTINE Allocate,
    PFREE_ROUTINE Free,
    PFILE_SET_SIZE_ROUTINE FileSetSize,
    PFILE_WRITE_ROUTINE FileWrite,
    PFILE_READ_ROUTINE FileRead,
    PFILE_FLUSH_ROUTINE FileFlush,
    ULONG Cluster OPTIONAL,
    PCUNICODE_STRING FileName OPTIONAL);

void CMAPI
HvFree(
   PHHIVE RegistryHive);

#define HvGetCell(Hive, Cell)   (Hive)->GetCellRoutine(Hive, Cell)

#define HvReleaseCell(Hive, Cell)               \
do {                                            \
    if ((Hive)->ReleaseCellRoutine)             \
        (Hive)->ReleaseCellRoutine(Hive, Cell); \
} while(0)

LONG CMAPI
HvGetCellSize(
   PHHIVE RegistryHive,
   void* Cell);

HCELL_INDEX CMAPI
HvAllocateCell(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage,
   IN HCELL_INDEX Vicinity);

BOOLEAN CMAPI
HvIsCellAllocated(
    IN PHHIVE RegistryHive,
    IN HCELL_INDEX CellIndex
);

HCELL_INDEX CMAPI
HvReallocateCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset,
   ULONG Size);

void CMAPI
HvFreeCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset);

BOOLEAN CMAPI
HvMarkCellDirty(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset,
   BOOLEAN HoldingLock);

BOOLEAN CMAPI
HvIsCellDirty(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

BOOLEAN
CMAPI
HvHiveWillShrink(
    IN PHHIVE RegistryHive
);

BOOLEAN CMAPI
HvSyncHive(
   PHHIVE RegistryHive);

BOOLEAN CMAPI
HvWriteHive(
   PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvWriteAlternateHive(
    _In_ PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvSyncHiveFromRecover(
    _In_ PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvTrackCellRef(
    IN OUT PHV_TRACK_CELL_REF CellRef,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

void
CMAPI
HvReleaseFreeCellRefArray(
    IN OUT PHV_TRACK_CELL_REF CellRef
);

/*
 * Private functions.
 */

PCELL_DATA CMAPI
HvpGetCellData(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CellIndex);

PHBIN CMAPI
HvpAddBin(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage);

int CMAPI
HvpCreateHiveFreeCellList(
   PHHIVE Hive);

ULONG CMAPI
HvpHiveHeaderChecksum(
   PHBASE_BLOCK HiveHeader);

BOOLEAN CMAPI
HvpVerifyHiveHeader(
    _In_ PHBASE_BLOCK BaseBlock,
    _In_ ULONG FileType);

//
// Registry Self-Heal Routines
//
BOOLEAN
CMAPI
CmIsSelfHealEnabled(
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairParentKey(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX TargetKey,
    _In_ HCELL_INDEX ParentKey,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairParentNode(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _In_ HCELL_INDEX ParentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairKeyNodeSignature(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairClassOfNodeKey(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairValueList(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairValueListCount(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG ListCountIndex,
    _Inout_ PCELL_DATA ValueListData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairSubKeyCounts(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG Count,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairSubKeyList(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

/* Old-style Public "Cmlib" functions */

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name);

/* NT-style Public Cm functions */

//
// Check Registry Routines
//
CM_CHECK_REGISTRY_STATUS
HvValidateBin(
    _In_ PHHIVE Hive,
    _In_ PHBIN Bin);

CM_CHECK_REGISTRY_STATUS
HvValidateHive(
    _In_ PHHIVE Hive);

CM_CHECK_REGISTRY_STATUS
CmCheckRegistry(
    _In_ PCMHIVE RegistryHive,
    _In_ ULONG Flags);

//
// Cell Index Routines
//
HCELL_INDEX
CmpFindSubKeyByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Parent,
    IN PCUNICODE_STRING SearchName
);

HCELL_INDEX
CmpFindSubKeyByNumber(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node,
    IN ULONG Number
);

ULONG
CmpComputeHashKey(
    IN ULONG Hash,
    IN PCUNICODE_STRING Name,
    IN BOOLEAN AllowSeparators
);

BOOLEAN
CmpAddSubKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Parent,
    IN HCELL_INDEX Child
);

BOOLEAN
CmpRemoveSubKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX ParentKey,
    IN HCELL_INDEX TargetKey
);

BOOLEAN
CmpMarkIndexDirty(
    IN PHHIVE Hive,
    HCELL_INDEX ParentKey,
    HCELL_INDEX TargetKey
);


//
// Name Functions
//
LONG
CmpCompareCompressedName(
    IN PCUNICODE_STRING SearchName,
    IN PWCHAR CompressedName,
    IN ULONG NameLength
);

USHORT
CmpNameSize(
    IN PHHIVE Hive,
    IN PCUNICODE_STRING Name
);

USHORT
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
);

USHORT
CmpCopyName(
    IN PHHIVE Hive,
    OUT PWCHAR Destination,
    IN PCUNICODE_STRING Source
);

void
CmpCopyCompressedName(
    OUT PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
);

BOOLEAN
CmpFindNameInList(
    IN PHHIVE Hive,
    IN PCHILD_LIST ChildList,
    IN PCUNICODE_STRING Name,
    OUT PULONG ChildIndex OPTIONAL,
    OUT PHCELL_INDEX CellIndex
);


//
// Cell Value Routines
//
HCELL_INDEX
CmpFindValueByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE KeyNode,
    IN PCUNICODE_STRING Name
);

PCELL_DATA
CmpValueToData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value,
    OUT PULONG Length
);

int
CmpSetValueDataNew(
    IN PHHIVE Hive,
    IN void* Data,
    IN ULONG DataSize,
    IN HSTORAGE_TYPE StorageType,
    IN HCELL_INDEX ValueCell,
    OUT PHCELL_INDEX DataCell
);

int
CmpAddValueToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ValueCell,
    IN ULONG Index,
    IN HSTORAGE_TYPE StorageType,
    IN OUT PCHILD_LIST ChildList
);

BOOLEAN
CmpFreeValue(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

BOOLEAN
CmpMarkValueDataDirty(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value
);

BOOLEAN
CmpFreeValueData(
    IN PHHIVE Hive,
    IN HCELL_INDEX DataCell,
    IN ULONG DataLength
);

int
CmpRemoveValueFromList(
    IN PHHIVE Hive,
    IN ULONG Index,
    IN OUT PCHILD_LIST ChildList
);

BOOLEAN
CmpGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value,
    OUT PULONG Length,
    OUT void* *Buffer,
    OUT unsigned char* BufferAllocated,
    OUT PHCELL_INDEX CellToRelease
);

int
CmpCopyKeyValueList(
    IN PHHIVE SourceHive,
    IN PCHILD_LIST SrcValueList,
    IN PHHIVE DestinationHive,
    IN OUT PCHILD_LIST DestValueList,
    IN HSTORAGE_TYPE StorageType
);

int
CmpFreeKeyByCell(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN BOOLEAN Unlink
);

void
CmpRemoveSecurityCellList(
    IN PHHIVE Hive,
    IN HCELL_INDEX SecurityCell
);

void
CmpFreeSecurityDescriptor(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

/******************************************************************************/

#endif /* _CMLIB_H_ */
