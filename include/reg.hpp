/*
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG
 * FILE:            ntreg.h
 * PURPOSE:         包含注册表操作相关的定义和宏
 */


#ifndef _NTREG_H_
#define _NTREG_H_

/* INCLUDES *****************************************************************/

#include "typedefs.hpp"
#include "cmi.hpp"
#include "cmlib.hpp"
#include "binhive.hpp"

/* DATA *********************************************************************/

/**
 * @brief 注册表Hive列表条目结构。
 *
 * 该结构用于存储注册表Hive的相关信息，包括Hive名称、注册表路径、Hive指针、安全描述符及其长度。
 */
typedef struct _HIVE_LIST_ENTRY
{
    const char *   HiveName; /**< Hive的名称。 */
    PCWSTR  HiveRegistryPath; /**< Hive在注册表中的路径。 */
    PCMHIVE CmHive; /**< Hive的指针。 */
    unsigned char*  SecurityDescriptor; /**< 安全描述符。 */
    ULONG   SecurityDescriptorLength; /**< 安全描述符的长度。 */
} HIVE_LIST_ENTRY, *PHIVE_LIST_ENTRY;

/**
 * @brief 注册表Hive的最大数量。
 */
#define MAX_NUMBER_OF_REGISTRY_HIVES    4

/**
 * @brief 注册表Hive列表。
 */
extern HIVE_LIST_ENTRY RegistryHives[];

/* 
 * 防止重复定义
 */
#undef ERROR_SUCCESS
#undef ERROR_INVALID_FUNCTION
#undef ERROR_FILE_NOT_FOUND             
#undef ERROR_ACCESS_DENIED              
#undef ERROR_NOT_ENOUGH_MEMORY         
#undef ERROR_GEN_FAILURE          
#undef ERROR_INVALID_PARAMETER     
#undef ERROR_MORE_DATA     
#undef ERROR_NO_MORE_ITEMS 
#undef ERROR_NO_LOG_SPACE      
#undef ERROR_NO_SYSTEM_RESOURCES

#undef REG_NONE                           
#undef REG_SZ                             
#undef REG_EXPAND_SZ                      
#undef REG_BINARY                         
#undef REG_DWORD                          
#undef REG_DWORD_LITTLEENDIAN            
#undef REG_DWORD_BIGENDIAN               
#undef REG_LINK                           
#undef REG_MULTI_SZ                       
#undef REG_RESOURCE_LIST                  
#undef REG_FULL_RESOURCE_DESCRIPTOR       
#undef REG_RESOURCE_REQUIREMENTS_LIST   
#undef REG_QWORD                
#undef REG_QWORD_LITTLEENDIAN      
/* END OF INCLUDES **********/


/**
 * @brief 错误代码定义。
 */
#define ERROR_SUCCESS                    0L
#define ERROR_INVALID_FUNCTION           1L
#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_NOT_ENOUGH_MEMORY          8L
#define ERROR_GEN_FAILURE                31L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L
#define ERROR_NO_LOG_SPACE               1019L
#define ERROR_NO_SYSTEM_RESOURCES        1450L

/**
 * @brief 注册表类型定义。
 */
#define REG_NONE                           0
#define REG_SZ                             1
#define REG_EXPAND_SZ                      2
#define REG_BINARY                         3
#define REG_DWORD                          4
#define REG_DWORD_LITTLEENDIAN            4
#define REG_DWORD_BIGENDIAN               5
#define REG_LINK                           6
#define REG_MULTI_SZ                       7
#define REG_RESOURCE_LIST                  8
#define REG_FULL_RESOURCE_DESCRIPTOR       9
#define REG_RESOURCE_REQUIREMENTS_LIST     10
#define REG_QWORD                          11
#define REG_QWORD_LITTLEENDIAN            11

/**
 * @brief Hive名称定义。
 */
#define  HIVE_HARDWARE		"HARDWARE"
#define  HIVE_SYSTEM		"SYSTEM"
#define  HIVE_SOFTWARE		"SOFTWARE"
#define  HIVE_SAM			"SAM"
#define  HIVE_SEC			"SECURITY"

/**
 * @brief 注册表根键路径定义。
 */
#define  REG_ROOT_KEY			u"\\NTReg"
#define  REG_MACHINE_KEY		u"\\NTReg\\Local"

/**
 * @brief 注册表硬件键路径定义。
 */
#define  REG_HARDWARE_KEY		u"\\NTReg\\Local\\HARDWARE"
#define  REG_SYSTEM_KEY			u"\\NTReg\\Local\\SYSTEM"
#define  REG_SOFTWARE_KEY		u"\\NTReg\\Local\\SOFTWARE"
#define  REG_SAM_KEY			u"\\NTReg\\Local\\SAM"
#define  REG_SEC_KEY			u"\\NTReg\\Local\\SECURITY"

/**
 * @brief 注册表描述键路径定义。
 */
#define  REG_DESCRIPTION_KEY	u"\\NTReg\\Local\\HARDWARE\\DESCRIPTION"
#define  REG_DEVICEMAP_KEY		u"\\NTReg\\Local\\HARDWARE\\DEVICEMAP"
#define  REG_RESOURCEMAP_KEY	u"\\NTReg\\Local\\HARDWARE\\RESOURCEMAP"
#define  REG_CLASSES_KEY		u"\\NTReg\\Local\\Software\\Classes"
#define  REG_CURVER_KEY         u"\\NTReg\\Local\\SYSTEM\\NTSoft\\NTReg\\CurrentVersion"

/**
 * @brief 注册表时间炸弹键路径定义。
 */
#define  REG_TIMEBOMB_KEY		u"\\NTReg\\Local\\SYSTEM\\TimeBomb"

/**
 * @brief 注册表用户键路径定义。
 */
#define  REG_USER_KEY			u"\\NTReg\\User"
#define  REG_DEFAULT_USER_KEY	u"\\NTReg\\User\\.Default"
#define  REG_CURRENT_USER_KEY	u"\\NTReg\\User\\CurrentUser"


/**
 * @brief 注册表键句柄定义。
 */
#if 1
#define REGHKEY_CLASSES_ROOT        ((REGHKEY)(LONG_PTR)(LONG)0x80000000) /**< 文件关联和COM对象注册信息。 */
#define REGHKEY_CURRENT_USER        ((REGHKEY)(LONG_PTR)(LONG)0x80000001) /**< 当前用户的配置信息。 */
#define REGHKEY_LOCAL_MACHINE       ((REGHKEY)(LONG_PTR)(LONG)0x80000002) /**< 系统的硬件和软件配置信息。 */
#define REGHKEY_USERS               ((REGHKEY)(LONG_PTR)(LONG)0x80000003) /**< 所有用户配置文件的信息。 */
#define REGHKEY_PERFORMANCE_DATA    ((REGHKEY)(LONG_PTR)(LONG)0x80000004) /**< 系统性能数据。 */
#define REGHKEY_CURRENT_CONFIG      ((REGHKEY)(LONG_PTR)(LONG)0x80000005) /**< 当前硬件配置文件的信息。 */
#define REGHKEY_DYN_DATA            ((REGHKEY)(LONG_PTR)(LONG)0x80000006) /**< 动态数据（在较新的Windows版本中已不常用）。 */
#define REGHKEY_PERFORMANCE_TEXT    ((REGHKEY)(LONG_PTR)(LONG)0x80000050) /**< 性能数据的文本信息（在较新的Windows版本中已不常用）。 */
#define REGHKEY_PERFORMANCE_NLSTEXT ((REGHKEY)(LONG_PTR)(LONG)0x80000060) /**< 性能数据的本地化文本信息（在较新的Windows版本中已不常用）。 */
#endif


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


/**
 * @brief 定义从<ntstatus.h>复制的常量。
 * 我们只希望包含主机头文件，因此手动定义这些常量。
 */
#define STATUS_SUCCESS                   ((int)0x00000000) ///< 操作成功。
#define STATUS_UNSUCCESSFUL              ((int)0xC0000001) ///< 操作不成功。
// #define STATUS_NOT_IMPLEMENTED           ((int)0xC0000002)
// #define STATUS_INVALID_PARAMETER         ((int)0xC000000D)
// #define STATUS_NO_MEMORY                 ((int)0xC0000017)
// #define STATUS_INSUFFICIENT_RESOURCES    ((int)0xC000009A)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((int)0xC0000034) ///< 对象名称未找到。
// #define STATUS_INVALID_PARAMETER_2       ((int)0xC00000F0)
// #define STATUS_BUFFER_OVERFLOW           ((int)0x80000005)

#define OBJ_NAME_PATH_SEPARATOR           ((WCHAR)u'\\') ///< 对象名称路径分隔符。

/**
 * @brief 计算绝对值。
 * @param V 输入值。
 * @return 输入值的绝对值。
 */
#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

/**
 * @brief 标记代码为分页代码。
 */
#define PAGED_CODE()

/**
 * @brief 查找掩码中第一个设置的位。
 * @param Index 存储找到的位索引。
 * @param Mask 要查找的掩码。
 * @return 如果找到位，返回非零值；否则返回零。
 */
unsigned char BitScanForward(ULONG * Index, unsigned long Mask);

/**
 * @brief 查找掩码中最后一个设置的位。
 * @param Index 存储找到的位索引。
 * @param Mask 要查找的掩码。
 * @return 如果找到位，返回非零值；否则返回零。
 */
unsigned char BitScanReverse(ULONG * const Index, unsigned long Mask);

/**
 * @brief 用一个32位值填充内存。
 * @param dst 目标内存地址。
 * @param len 要填充的长度。
 * @param val 填充的32位值。
 */
#define RtlFillMemoryUlong(dst, len, val) memset(dst, val, len)


/* #ifdef _M_AMD64
    #define BitScanForward64 _BitScanForward64
    #define BitScanReverse64 _BitScanReverse64
#endif */
#define BitScanForward64 BitScanForward
#define BitScanReverse64 BitScanReverse

#ifndef _MSC_VER
#ifndef _countof
/**
 * @brief 计算数组元素的数量。
 * @param _Array 数组。
 * @return 数组元素的数量。
 */
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif
#endif

/**
 * @brief 注册表蜂巢列表头。
 */
extern LIST_ENTRY CmiHiveListHead;

/**
 * @brief 注册表安全访问掩码类型。
 */
typedef DWORD REGSAM;

/**
 * @brief 注册表句柄类型。
 */
typedef HANDLE REGHKEY, *PREGHKEY;


/* FUNCTIONS ****************************************************************/

BOOLEAN
RegInitializeRegistry(
    const char * HiveList,
    BOOLEAN LoadHiveData);

void
RegShutdownRegistry(void);


LONG 
NTRegQueryValueExW(
    REGHKEY hKey,
    LPCWSTR lpValueName,
    PULONG lpReserved,
    OUT PULONG lpType OPTIONAL,
    OUT unsigned char* lpData OPTIONAL,
    OUT PULONG lpcbData OPTIONAL);

LONG 
NTRegSetValueExW(
    REGHKEY hKey,
    LPCWSTR lpValueName OPTIONAL,
    ULONG Reserved,
    ULONG dwType,
    const unsigned char* lpData,
    ULONG cbData);

LONG 
NTRegCloseKey(
    REGHKEY hKey);

LONG 
NTRegDeleteKeyW(
    REGHKEY hKey,
    LPCWSTR lpSubKey);

LONG
NTRegDeleteValueW(
    REGHKEY hKey,
    LPCWSTR lpValueName OPTIONAL);

LONG
NTRegEnumValue(
    REGHKEY Key,
    ULONG Index,
    OUT PWCHAR ValueName,
    IN OUT PULONG NameSize,
    OUT PULONG Type OPTIONAL,
    OUT unsigned char* Data OPTIONAL,
    IN OUT PULONG DataSize OPTIONAL);

LONG
NTRegCreateKeyW(
    REGHKEY hKey,
    LPCWSTR lpSubKey,
    OUT PREGHKEY phkResult);

LONG 
NTRegCreateKeyExW(
    REGHKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass OPTIONAL,
    DWORD dwOptions,
    REGSAM samDesired,
    void* lpSecurityAttributes OPTIONAL,
    OUT PREGHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL);

LONG
NTRegEnumKey(
    REGHKEY Key,
    ULONG Index,
    OUT PWCHAR Name,
    OUT PULONG NameSize,
    OUT PREGHKEY SubKey OPTIONAL);

LONG 
NTRegOpenKeyW(
    REGHKEY hKey,
    LPCWSTR lpSubKey,
    OUT PREGHKEY phkResult);

BOOLEAN
NTRegConnect();

void
NTRegDisconnect();


typedef 
struct _NTReg
{
    BOOLEAN (*Connect)();
    void (*Disconnect)();

    LONG (*OpenKey)
    (
        REGHKEY hKey,
        LPCWSTR lpSubKey,
        OUT PREGHKEY phkResult);

    LONG (*CloseKey)(REGHKEY hKey);

    LONG (*CreateKey)
    (
        REGHKEY hKey,
        LPCWSTR lpSubKey,
        OUT PREGHKEY phkResult);

    LONG (*CreateKeyEx)
    (
        REGHKEY hKey,
        LPCWSTR lpSubKey,
        DWORD Reserved,
        LPWSTR lpClass OPTIONAL,
        DWORD dwOptions,
        REGSAM samDesired,
        void* lpSecurityAttributes OPTIONAL,
        OUT PREGHKEY phkResult,
        OUT LPDWORD lpdwDisposition OPTIONAL);
    
    LONG (*DeleteKey)
    (    
        REGHKEY hKey,
        LPCWSTR lpSubKey);

    LONG (*EnumKey)
    (
        REGHKEY Key,
        ULONG Index,
        OUT PWCHAR Name,
        OUT PULONG NameSize,
        OUT PREGHKEY SubKey OPTIONAL);
    
    LONG (*QueryValue)
    (
        REGHKEY hKey,
        LPCWSTR lpValueName,
        PULONG lpReserved,
        OUT PULONG lpType OPTIONAL,
        OUT unsigned char* lpData OPTIONAL,
        OUT PULONG lpcbData OPTIONAL);

    LONG (*SetValue)
    (
        REGHKEY hKey,
        LPCWSTR lpValueName OPTIONAL,
        ULONG Reserved,
        ULONG dwType,
        const unsigned char* lpData,
        ULONG cbData);
    
    LONG (*DeleteValue)
    (    
        REGHKEY hKey,
        LPCWSTR lpValueName OPTIONAL);

    LONG (*EnumValue)
    (
        REGHKEY Key,
        ULONG Index,
        OUT PWCHAR ValueName,
        OUT PULONG NameSize,
        OUT PULONG Type OPTIONAL,
        OUT unsigned char* Data OPTIONAL,
        OUT PULONG DataSize OPTIONAL);
    
} NTReg;

NTReg* InitNTReg();


#endif /* _NTREG_H_ */

/* EOF */
