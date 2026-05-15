#define NDEBUG

#include "cmlib.hpp"
#include "cmi.hpp"
#include "debug.hpp"

/* STRUCTURES ****************************************************************/

/**
 * @brief 注册表堆栈工作状态结构体
 *
 * 该结构体用于存储注册表堆栈工作状态的相关信息。
 */
typedef struct _CMP_REGISTRY_STACK_WORK_STATE
{
    ULONG ChildCellIndex; /**< 子单元格索引 */
    HCELL_INDEX Parent; /**< 父单元格偏移量 */
    HCELL_INDEX Current; /**< 当前单元格偏移量 */
    HCELL_INDEX Sibling; /**< 兄弟单元格偏移量 */
} CMP_REGISTRY_STACK_WORK_STATE, *PCMP_REGISTRY_STACK_WORK_STATE;


/* DEFINES  ******************************************************************/

/**
 * @brief 获取HHIVE结构的宏定义
 *
 * 该宏定义用于获取HHIVE结构的指针。
 */
#define GET_HHIVE(CmHive) (&((CmHive)->Hive))

/**
 * @brief 获取HHIVE根单元格的宏定义
 *
 * 该宏定义用于获取HHIVE结构的根单元格。
 */
#define GET_HHIVE_ROOT_CELL(Hive) ((Hive)->BaseBlock->RootCell)

/**
 * @brief 获取HHIVE存储块的宏定义
 *
 * 该宏定义用于获取HHIVE结构的存储块。
 */
#define GET_HHIVE_BIN(Hive, StorageIndex, BlockIndex) ((PHBIN)Hive->Storage[StorageIndex].BlockList[BlockIndex].BinAddress)

/**
 * @brief 获取单元格存储块的宏定义
 *
 * 该宏定义用于获取单元格的存储块。
 */
#define GET_CELL_BIN(Bin) ((PHCELL)((unsigned char*)Bin + sizeof(HBIN)))

/**
 * @brief 判断单元格是否为易失性的宏定义
 *
 * 该宏定义用于判断单元格是否为易失性。
 */
#define IS_CELL_VOLATILE(Cell) (HvGetCellType(Cell) == Volatile)

/**
 * @brief 易失性蜂巢结构的指针
 *
 * 该外部变量用于存储易失性蜂巢结构的指针。
 */
PCMHIVE CmiVolatileHive;


/**
 * @brief 堆栈优先级的宏定义
 *
 * 该宏定义用于定义堆栈的优先级。
 */
#define CMP_PRIOR_STACK 1

/**
 * @brief 注册表最大树深度级别的宏定义
 *
 * 该宏定义用于定义注册表最大树深度级别。
 */
#define CMP_REGISTRY_MAX_LEVELS_TREE_DEPTH 512

/**
 * @brief 键大小阈值的宏定义
 *
 * 该宏定义用于定义键大小的阈值。
 */
#define CMP_KEY_SIZE_THRESHOLD 0x45C

/**
 * @brief 易失性列表未初始化的宏定义
 *
 * 该宏定义用于定义易失性列表未初始化的状态。
 */
#define CMP_VOLATILE_LIST_UNINTIALIZED 0xBAADF00D


/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief 验证子键的词法顺序
 *
 * 该函数用于验证子键与其前一个兄弟键之间的词法顺序。
 *
 * @param Hive 蜂巢结构的指针
 * @param Child 子键单元格偏移量
 * @param Sibling 前一个兄弟键单元格偏移量
 * @return BOOLEAN 如果顺序合法返回TRUE，否则返回FALSE
 */
static
BOOLEAN
CmpValidateLexicalOrder(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX Child,
    _In_ HCELL_INDEX Sibling)
{
    LONG Result;
    UNICODE_STRING ChildString, SiblingString;
    PCM_KEY_NODE ChildNode, SiblingNode;

    PAGED_CODE();

    /* 获取子键节点 */
    ChildNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Child));
    if (!ChildNode)
    {
        DPRINT1("Failed to get the child node\n");
        return FALSE;
    }

    /* 获取兄弟键节点 */
    SiblingNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Sibling));
    if (!SiblingNode)
    {
        DPRINT1("Failed to get the sibling node\n");
        return FALSE;
    }

    /* 情况1：两个非压缩的Unicode名称 */
    if ((ChildNode->Flags & KEY_COMP_NAME) == 0 &&
        (SiblingNode->Flags & KEY_COMP_NAME) == 0)
    {
        SiblingString.Buffer = &(SiblingNode->Name[0]);
        SiblingString.Length = SiblingNode->NameLength;
        SiblingString.MaximumLength = SiblingNode->NameLength;

        ChildString.Buffer = &(ChildNode->Name[0]);
        ChildString.Length = ChildNode->NameLength;
        ChildString.MaximumLength = ChildNode->NameLength;

        Result = RtlCompareUnicodeString(&SiblingString, &ChildString, TRUE);
        if (Result >= 0)
        {
            DPRINT1("The sibling node name is greater or equal to that of the child\n");
            return FALSE;
        }
    }

    /* 情况2：两个压缩的Unicode名称 */
    if ((ChildNode->Flags & KEY_COMP_NAME) &&
        (SiblingNode->Flags & KEY_COMP_NAME))
    {
        DPRINT("Lexicographical order checks for two compressed names is UNIMPLEMENTED, assume the key is healthy...\n");
        return TRUE;
    }

    /* 情况3：子键名称是压缩的，但兄弟键名称不是 */
    if ((ChildNode->Flags & KEY_COMP_NAME) &&
        (SiblingNode->Flags & KEY_COMP_NAME) == 0)
    {
        SiblingString.Buffer = &(SiblingNode->Name[0]);
        SiblingString.Length = SiblingNode->NameLength;
        SiblingString.MaximumLength = SiblingNode->NameLength;
        Result = CmpCompareCompressedName(&SiblingString,
                                          ChildNode->Name,
                                          ChildNode->NameLength);
        if (Result >= 0)
        {
            DPRINT1("The sibling node name is greater or equal to that of the compressed child\n");
            return FALSE;
        }
    }

    /* 情况4：兄弟键名称是压缩的，但子键名称不是 */
    if ((SiblingNode->Flags & KEY_COMP_NAME) &&
        (ChildNode->Flags & KEY_COMP_NAME) == 0)
    {
        ChildString.Buffer = &(ChildNode->Name[0]);
        ChildString.Length = ChildNode->NameLength;
        ChildString.MaximumLength = ChildNode->NameLength;
        Result = CmpCompareCompressedName(&ChildString,
                                          SiblingNode->Name,
                                          SiblingNode->NameLength);
        if (Result <= 0)
        {
            DPRINT1("The compressed sibling node name is lesser or equal to that of the child\n");
            return FALSE;
        }
    }

    return TRUE;
}


/**
 * @brief 验证键的类
 *
 * 该函数用于验证给定键单元格的类。
 *
 * @param Hive 蜂巢结构的指针
 * @param CurrentCell 当前键单元格偏移量
 * @param CellData 当前键单元格数据的指针
 * @return CM_CHECK_REGISTRY_STATUS 如果类正常返回CM_CHECK_REGISTRY_GOOD，如果类未分配返回CM_CHECK_REGISTRY_KEY_CLASS_UNALLOCATED
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateClass(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData)
{
    ULONG ClassLength;
    HCELL_INDEX ClassCell;

    PAGED_CODE();

    ASSERT(CurrentCell != HCELL_NIL);
    ASSERT(CellData);

    /* 缓存类单元格并验证它（如果有） */
    ClassCell = CellData->u.KeyNode.Class;
    ClassLength = CellData->u.KeyNode.ClassLength;
    if (ClassLength > 0)
    {
        if (ClassCell == HCELL_NIL)
        {
            DPRINT1("The key node class is NIL but the class length is not 0, resetting it\n");
            HvMarkCellDirty(Hive, CurrentCell, FALSE);
            CellData->u.KeyNode.ClassLength = 0;
            return CM_CHECK_REGISTRY_GOOD;
        }

        if (!HvIsCellAllocated(Hive, ClassCell))
        {
            DPRINT1("The key class is not allocated\n");
            return CM_CHECK_REGISTRY_KEY_CLASS_UNALLOCATED;
        }
    }

    return CM_CHECK_REGISTRY_GOOD;
}


/**
 * @brief 通过计数验证值列表
 *
 * 该函数通过计数验证值列表中的每个值。如果某个值损坏，则将其从列表中移除，并进行自我修复。
 *
 * @param Hive 蜂巢结构的指针
 * @param CurrentCell 当前键单元格偏移量
 * @param ListCount 列表计数，描述列表中实际的值数量
 * @param ValueListData 当前键单元格数据的指针，包含要验证的值列表
 * @param ValuesRemoved 输出参数，函数完成后包含从列表中移除的值数量
 * @param FixHive 如果设置为TRUE，目标蜂巢将被修复
 * @return CM_CHECK_REGISTRY_STATUS 返回值列表的状态
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateValueListByCount(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG ListCount,
    _In_ PCELL_DATA ValueListData,
    _Out_ PULONG ValuesRemoved,
    _In_ BOOLEAN FixHive)
{
    ULONG ValueDataSize;
    ULONG ListCountIndex;
    ULONG DataSize;
    HCELL_INDEX DataCell;
    HCELL_INDEX ValueCell;
    PCELL_DATA ValueData;
    ULONG ValueNameLength, TotalValueNameLength;

    PAGED_CODE();

    ASSERT(ValueListData);
    ASSERT(ListCount != 0);

    /* 假设我们还没有移除任何值计数 */
    *ValuesRemoved = 0;

    /* 开始循环每个值并验证它 */
    ListCountIndex = 0;
    while (ListCountIndex < ListCount)
    {
        ValueCell = ValueListData->u.KeyList[ListCountIndex];
        if (ValueCell == HCELL_NIL)
        {
            if (!CmpRepairValueListCount(Hive,
                                         CurrentCell,
                                         ListCountIndex,
                                         ValueListData,
                                         FixHive))
            {
                DPRINT1("The value cell is NIL (at index %lu, list count %lu)\n",
                        ListCountIndex, ListCount);
                return CM_CHECK_REGISTRY_VALUE_CELL_NIL;
            }

            /* 减少列表计数并继续下一个值 */
            ListCount--;
            *ValuesRemoved++;
            DPRINT1("Damaged value removed, continuing with the next value...\n");
            continue;
        }

        if (!HvIsCellAllocated(Hive, ValueCell))
        {
            if (!CmpRepairValueListCount(Hive,
                                         CurrentCell,
                                         ListCountIndex,
                                         ValueListData,
                                         FixHive))
            {
                DPRINT1("The value cell is not allocated (at index %lu, list count %lu)\n",
                        ListCountIndex, ListCount);
                return CM_CHECK_REGISTRY_VALUE_CELL_UNALLOCATED;
            }

            /* 减少列表计数并继续下一个值 */
            ListCount--;
            *ValuesRemoved++;
            DPRINT1("Damaged value removed, continuing with the next value...\n");
            continue;
        }

        /* 从该值获取单元格数据 */
        ValueData = reinterpret_cast<PCELL_DATA>(HvGetCell(Hive, ValueCell));
        if (!ValueData)
        {
            DPRINT1("Cell data of the value cell not found (at index %lu, value count %lu)\n",
                    ListCountIndex, ListCount);
            return CM_CHECK_REGISTRY_VALUE_CELL_DATA_NOT_FOUND;
        }

        /* 检查值大小是否合理 */
        ValueDataSize = HvGetCellSize(Hive, ValueData);
        ValueNameLength = ValueData->u.KeyValue.NameLength;
        TotalValueNameLength = ValueNameLength + FIELD_OFFSET(CM_KEY_VALUE, Name);
        if (TotalValueNameLength > ValueDataSize)
        {
            if (!CmpRepairValueListCount(Hive,
                                         CurrentCell,
                                         ListCountIndex,
                                         ValueListData,
                                         FixHive))
            {
                DPRINT1("The total size is bigger than the actual cell size (total size %lu, cell size %lu, at index %lu)\n",
                        TotalValueNameLength, ValueDataSize, ListCountIndex);
                return CM_CHECK_REGISTRY_VALUE_CELL_SIZE_NOT_SANE;
            }

            /* 减少列表计数并继续下一个值 */
            ListCount--;
            *ValuesRemoved++;
            DPRINT1("Damaged value removed, continuing with the next value...\n");
            continue;
        }

        /* 值单元格大小合理，最后验证值单元格的实际数据 */
        DataCell = ValueData->u.KeyValue.Data;
        if (!CmpIsKeyValueSmall(&DataSize, ValueData->u.KeyValue.DataLength))
        {
            /* 验证实际数据的大小 */
            if (DataSize == 0)
            {
                if (DataCell != HCELL_NIL)
                {
                    if (!CmpRepairValueListCount(Hive,
                                                 CurrentCell,
                                                 ListCountIndex,
                                                 ValueListData,
                                                 FixHive))
                    {
                        DPRINT1("The data is not NIL on a 0 length, data is corrupt\n");
                        return CM_CHECK_REGISTRY_CORRUPT_VALUE_DATA;
                    }

                    /* 减少列表计数并继续下一个值 */
                    ListCount--;
                    *ValuesRemoved++;
                    DPRINT1("Damaged value removed, continuing with the next value...\n");
                    continue;
                }
            }
            else
            {
                if (!HvIsCellAllocated(Hive, DataCell))
                {
                    if (!CmpRepairValueListCount(Hive,
                                                 CurrentCell,
                                                 ListCountIndex,
                                                 ValueListData,
                                                 FixHive))
                    {
                        DPRINT1("The data is not NIL on a 0 length, data is corrupt\n");
                        return CM_CHECK_REGISTRY_DATA_CELL_NOT_ALLOCATED;
                    }

                    /* 减少列表计数并继续下一个值 */
                    ListCount--;
                    *ValuesRemoved++;
                    DPRINT1("Damaged value removed, continuing with the next value...\n");
                    continue;
                }
            }

            if (CmpIsKeyValueBig(Hive, DataSize))
            {
                /* Big value: validate the data cell is allocated */
                if (!HvIsCellAllocated(Hive, DataCell))
                {
                    if (!CmpRepairValueListCount(Hive,
                                                 CurrentCell,
                                                 ListCountIndex,
                                                 ValueListData,
                                                 FixHive))
                    {
                        DPRINT1("Big value data cell not allocated\n");
                        return CM_CHECK_REGISTRY_DATA_CELL_NOT_ALLOCATED;
                    }

                    ListCount--;
                    *ValuesRemoved++;
                    DPRINT1("Damaged big value removed\n");
                    continue;
                }
            }
        }

        /* 值的签名是否有效？ */
        if (ValueData->u.KeyValue.Signature != CM_KEY_VALUE_SIGNATURE)
        {
            if (!CmpRepairValueListCount(Hive,
                                         CurrentCell,
                                         ListCountIndex,
                                         ValueListData,
                                         FixHive))
            {
                DPRINT1("The key value signature is invalid\n");
                return CM_CHECK_REGISTRY_BAD_KEY_VALUE_SIGNATURE;
            }

            /* 减少列表计数并继续下一个值 */
            ListCount--;
            *ValuesRemoved++;
            DPRINT1("Damaged value removed, continuing with the next value...\n");
            continue;
        }

        /* 继续下一个值 */
        ListCountIndex++;
    }

    return CM_CHECK_REGISTRY_GOOD;
}


/**
 * @brief 验证值列表
 *
 * 该函数用于验证键的值列表。如果列表由于损坏而被破坏，则整个列表将被清除。该函数执行自我修复过程。
 *
 * @param Hive 蜂巢结构的指针
 * @param CurrentCell 当前键单元格偏移量
 * @param CellData 当前单元格数据的指针，包含值列表
 * @param FixHive 如果设置为TRUE，目标蜂巢将被修复
 * @return CM_CHECK_REGISTRY_STATUS 返回值列表的状态
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateValueList(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    ULONG TotalValueLength, ValueSize;
    ULONG ValueListCount;
    ULONG ValuesRemoved;
    HCELL_INDEX ValueListCell;
    PCELL_DATA ValueListData;

    PAGED_CODE();

    ASSERT(CurrentCell != HCELL_NIL);
    ASSERT(CellData);

    /* 缓存值列表并验证它 */
    ValueListCell = CellData->u.KeyNode.ValueList.List;
    ValueListCount = CellData->u.KeyNode.ValueList.Count;
    if (ValueListCount > 0)
    {
        if (!HvIsCellAllocated(Hive, ValueListCell))
        {
            DPRINT1("The value list is not allocated\n");
            return CM_CHECK_REGISTRY_VALUE_LIST_UNALLOCATED;
        }

        /* 从值列表单元格获取单元格数据 */
        ValueListData = reinterpret_cast<PCELL_DATA>(HvGetCell(Hive, ValueListCell));
        if (!ValueListData)
        {
            DPRINT1("Could not get cell data from the value list\n");
            return CM_CHECK_REGISTRY_VALUE_LIST_DATA_NOT_FOUND;
        }

        /* 缓存值大小和总长度，并确保这是一个合理的值列表 */
        ValueSize = HvGetCellSize(Hive, ValueListData);
        TotalValueLength = ValueListCount * sizeof(HCELL_INDEX);
        if (TotalValueLength > ValueSize)
        {
            DPRINT1("The value list is bigger than the cell (value list size %lu, cell size %lu)\n",
                    TotalValueLength, ValueSize);
            return CM_CHECK_REGISTRY_VALUE_LIST_SIZE_NOT_SANE;
        }

        /* 值列表是合理的，现在需要通过计数验证实际的列表 */
        CmStatusCode = CmpValidateValueListByCount(Hive,
                                                   CurrentCell,
                                                   ValueListCount,
                                                   ValueListData,
                                                   &ValuesRemoved,
                                                   FixHive);
        if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
        {
            DPRINT1("One of the values is corrupt and couldn't be repaired\n");
            return CmStatusCode;
        }

        /* 记录从列表中移除的值数量 */
        if (ValuesRemoved > 0)
        {
            DPRINT1("Values removed in the list -- %lu\n", ValuesRemoved);
        }
    }

    return CM_CHECK_REGISTRY_GOOD;
}

/**
 * @brief
 * 验证一个键的子键列表。如果列表因损坏而受损，该函数可以选择修复此列表或清除整个列表。
 * 该函数根据不同的存储类型执行不同的验证步骤。
 *
 * @param[in] Hive
 * 指向要验证子键列表的蜂巢描述符的指针。
 *
 * @param[in] CurrentCell
 * 当前子键列表指向的键单元格。
 *
 * @param[in] CellData
 * 当前单元格的单元格数据，子键列表由此而来。
 *
 * @param[in] FixHive
 * 如果设置为TRUE，目标蜂巢将被修复。
 *
 * @param[out] DoRepair
 * 指向布尔值的指针，由函数本身设置。函数会自动将其设置为FALSE，表示无法对列表本身进行修复。
 * 如果列表可以修复，则函数会将其设置为TRUE。详见备注。
 *
 * @return
 * 如果子键列表完好无损，返回CM_CHECK_REGISTRY_GOOD。
 * 如果易失性存储有稳定数据（这种情况不应发生），返回CM_CHECK_REGISTRY_STABLE_KEYS_ON_VOLATILE。
 * 如果子键列表单元格未分配，返回CM_CHECK_REGISTRY_SUBKEYS_LIST_UNALLOCATED。
 * 如果无法从子键列表单元格映射键索引，返回CM_CHECK_REGISTRY_CORRUPT_SUBKEYS_INDEX。
 * 如果键索引是叶子且子键计数与叶子不匹配，返回CM_CHECK_REGISTRY_BAD_SUBKEY_COUNT。
 * 如果列表中特定索引的键索引单元格未分配，返回CM_CHECK_REGISTRY_KEY_INDEX_CELL_UNALLOCATED。
 * 如果无法从索引映射叶子，返回CM_CHECK_REGISTRY_CORRUPT_LEAF_ON_ROOT。
 * 如果叶子有无效签名，返回CM_CHECK_REGISTRY_CORRUPT_LEAF_SIGNATURE。
 * 如果键索引有无效签名（即不是叶子也不是根），返回CM_CHECK_REGISTRY_CORRUPT_KEY_INDEX_SIGNATURE。
 *
 * @remarks
 * 在特定情况下可以进行深度子键列表修复，其中只有子键不影响键本身。
 * 函数会通过将DoRepair参数设置为TRUE来标记子键列表为可修复，调用者负责通过清除整个子键列表来修复键。
 * 如果损坏严重，以至于键本身也可能受损，则不进行修复。
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateSubKeyList(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive,
    _Out_ unsigned char* DoRepair)
{
    ULONG SubKeyCounts;
    HCELL_INDEX KeyIndexCell, SubKeysListCell;
    PCM_KEY_INDEX RootKeyIndex, LeafKeyIndex;
    ULONG RootIndex;
    ULONG TotalLeafCount;

    PAGED_CODE();

    ASSERT(CurrentCell != HCELL_NIL);
    ASSERT(CellData);

    RootKeyIndex = NULL;
    LeafKeyIndex = NULL;
    TotalLeafCount = 0;

    /*
     * 假设调用者不应进行任何子键列表的修复，除非我们明确同意。
     */
    *DoRepair = FALSE;

    /*
     * 对于易失性键，它们的数据可能会波动和变化，因此我们无法验证这些数据。
     * 但我们仍然希望易失性键不会被外部因素损坏，例如在易失性空间中有稳定键。
     */
    if (IS_CELL_VOLATILE(CurrentCell))
    {
        if (CellData->u.KeyNode.SubKeyCounts[Stable] != 0)
        {
            DPRINT1("The volatile key has stable subkeys\n");
            return CM_CHECK_REGISTRY_STABLE_KEYS_ON_VOLATILE;
        }

        return CM_CHECK_REGISTRY_GOOD;
    }

    /*
     * 这不是易失性键，缓存子键列表并验证它。
     */
    SubKeysListCell = CellData->u.KeyNode.SubKeyLists[Stable];
    SubKeyCounts = CellData->u.KeyNode.SubKeyCounts[Stable];
    if (SubKeyCounts > 0)
    {
        if (!HvIsCellAllocated(Hive, SubKeysListCell))
        {
            DPRINT1("The subkeys list cell is not allocated\n");
            *DoRepair = TRUE;
            return CM_CHECK_REGISTRY_SUBKEYS_LIST_UNALLOCATED;
        }

        /* 获取根索引并验证它 */
        RootKeyIndex = reinterpret_cast<PCM_KEY_INDEX>(HvGetCell(Hive, SubKeysListCell));
        if (!RootKeyIndex)
        {
            DPRINT1("Could not get the root key index of the subkeys list cell\n");
            return CM_CHECK_REGISTRY_CORRUPT_SUBKEYS_INDEX;
        }

        /*
         * 对于简单、快速和散列的叶子，我们希望相应的根索引计数与子键计数本身匹配。
         * 如果不是这种情况，我们可以隔离问题并修复计数。
         */
        if (RootKeyIndex->Signature == CM_KEY_INDEX_LEAF ||
            RootKeyIndex->Signature == CM_KEY_FAST_LEAF ||
            RootKeyIndex->Signature == CM_KEY_HASH_LEAF)
        {
            if (SubKeyCounts != RootKeyIndex->Count)
            {
                if (!CmpRepairSubKeyCounts(Hive,
                                           CurrentCell,
                                           RootKeyIndex->Count,
                                           CellData,
                                           FixHive))
                {
                    DPRINT1("The subkeys list has invalid count (subkeys count %lu, root key index count %lu)\n",
                            SubKeyCounts, RootKeyIndex->Count);
                    return CM_CHECK_REGISTRY_BAD_SUBKEY_COUNT;
                }
            }

            return CM_CHECK_REGISTRY_GOOD;
        }

        /*
         * 根索引不是叶子，检查索引是否是实际的根。
         */
        if (RootKeyIndex->Signature == CM_KEY_INDEX_ROOT)
        {
            /*
             * 对于根，我们必须遍历其中的每个叶子，并在确定叶子的有效性后增加根中的总叶子计数。
             * 这样我们可以看到子键列表计数是否与子键计数匹配。
             */
            for (RootIndex = 0; RootIndex < RootKeyIndex->Count; RootIndex++)
            {
                KeyIndexCell = RootKeyIndex->List[RootIndex];
                if (!HvIsCellAllocated(Hive, KeyIndexCell))
                {
                    DPRINT1("The key index cell is not allocated at index %lu\n", RootIndex);
                    *DoRepair = TRUE;
                    return CM_CHECK_REGISTRY_KEY_INDEX_CELL_UNALLOCATED;
                }

                /* 从根获取叶子 */
                LeafKeyIndex = reinterpret_cast<PCM_KEY_INDEX>(HvGetCell(Hive, KeyIndexCell));
                if (!LeafKeyIndex)
                {
                    DPRINT1("The root key index's signature is invalid!\n");
                    return CM_CHECK_REGISTRY_CORRUPT_LEAF_ON_ROOT;
                }

                /* 检查叶子是否有有效的签名 */
                if (LeafKeyIndex->Signature != CM_KEY_INDEX_LEAF &&
                    LeafKeyIndex->Signature != CM_KEY_FAST_LEAF &&
                    LeafKeyIndex->Signature != CM_KEY_HASH_LEAF)
                {
                    DPRINT1("The leaf's signature is invalid!\n");
                    *DoRepair = TRUE;
                    return CM_CHECK_REGISTRY_CORRUPT_LEAF_SIGNATURE;
                }

                /* 增加叶子的计数 */
                TotalLeafCount += LeafKeyIndex->Count;
            }

            /*
             * 我们已经建立了总叶子计数，我们必须确定这个计数是否与子键列表计数完全相同。
             * 否则，修复它。
             */
            if (SubKeyCounts != TotalLeafCount)
            {
                if (!CmpRepairSubKeyCounts(Hive,
                                           CurrentCell,
                                           TotalLeafCount,
                                           CellData,
                                           FixHive))
                {
                    DPRINT1("The subkeys list has invalid count (subkeys count %lu, total leaf count %lu)\n",
                            SubKeyCounts, TotalLeafCount);
                    return CM_CHECK_REGISTRY_BAD_SUBKEY_COUNT;
                }
            }

            return CM_CHECK_REGISTRY_GOOD;
        }

        /*
         * 根索引的签名无效。根据定义，整个子键列表完全损坏。
         */
        DPRINT1("The root key index's signature is invalid\n");
        *DoRepair = TRUE;
        return CM_CHECK_REGISTRY_CORRUPT_KEY_INDEX_SIGNATURE;
    }

    /* 如果我们到达这里，则该键没有子键 */
    return CM_CHECK_REGISTRY_GOOD;
}



/**
 * @brief
 * 清除注册表配置单元的易失性存储。此操作主要在系统启动期间完成。
 *
 * @param[in] Hive
 * 指向要清除易失性存储的蜂巢描述符的指针。
 *
 * @param[in] CurrentCell
 * 当前键单元格，蜂巢的易失性存储指向该单元格。
 *
 * @param[in] CellData
 * 当前单元格的单元格数据，易失性子键存储由此而来。
 *
 * @param[in] Flags
 * 一个位掩码标志，用于影响清除操作的执行方式。详见CmCheckRegistry文档。
 */
static
void
CmpPurgeVolatiles(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ ULONG Flags)
{
    PAGED_CODE();

    ASSERT(CellData);

    /* 调用者是否要求清除易失性存储？ */
    if (((Flags & CM_CHECK_REGISTRY_PURGE_VOLATILES) ||
         (Flags & CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES)) &&
        (CellData->u.KeyNode.SubKeyCounts[Volatile] != 0))
    {
        /*
         * 好的，调用者希望从该蜂巢中清除易失性存储。
         * 对于XP Beta 1或更新版本的蜂巢，我们不初始化整个易失性子键列表。
         * 对于旧版本的蜂巢，我们只进行清理。
         */
#if !defined(_BLDR_)
        HvMarkCellDirty(Hive, CurrentCell, FALSE);
#endif
        if ((Flags & CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES) &&
            (Hive->Version >= HSYS_WHISTLER_BETA1))
        {
            CellData->u.KeyNode.SubKeyLists[Volatile] = CMP_VOLATILE_LIST_UNINTIALIZED;
        }
        else
        {
            CellData->u.KeyNode.SubKeyLists[Volatile] = HCELL_NIL;
        }

        /* 清除计数 */
        CellData->u.KeyNode.SubKeyCounts[Volatile] = 0;
    }
}


/**
 * @brief
 * 验证注册表中的键单元格，确保键有效且未损坏。
 *
 * @param[in] Hive
 * 指向要验证键的注册表蜂巢描述符的指针。
 *
 * @param[in] SecurityDefaulted
 * 如果调用者设置为TRUE，表示蜂巢的安全属性由于修复而默认。
 * 如果设置为FALSE，蜂巢具有自己的安全细节。此参数目前未使用。
 *
 * @param[in] ParentCell
 * 当前单元格之前的父键单元格。如果第一个单元格是根单元格，则此参数可以是HCELL_NIL。
 *
 * @param[in] CurrentCell
 * 要验证的当前子键单元格。
 *
 * @param[in] Flags
 * 一个位掩码标志，用于影响易失性键存储中易失性键的清除操作。详见CmCheckRegistry文档。
 *
 * @param[in] FixHive
 * 如果设置为TRUE，目标蜂巢将被修复。
 *
 * @return
 * 如果验证的键有效且未损坏，返回CM_CHECK_REGISTRY_GOOD。
 * 如果键单元格未分配，返回CM_CHECK_REGISTRY_KEY_CELL_NOT_ALLOCATED。
 * 如果无法从键单元格映射单元格数据，返回CM_CHECK_REGISTRY_CELL_DATA_NOT_FOUND。
 * 如果键单元格大小异常，超过验证检查允许的阈值，返回CM_CHECK_REGISTRY_CELL_SIZE_NOT_SANE。
 * 如果键节点名称长度为0，表示键没有名称，返回CM_CHECK_REGISTRY_KEY_NAME_LENGTH_ZERO。
 * 如果键大于单元格本身，返回CM_CHECK_REGISTRY_KEY_TOO_BIG_THAN_CELL。
 * 如果键的父节点不一致且无法修复，返回CM_CHECK_REGISTRY_BAD_KEY_NODE_PARENT。
 * 如果键节点签名损坏且无法修复，返回CM_CHECK_REGISTRY_BAD_KEY_NODE_SIGNATURE。
 * 否则返回失败的CM状态码。
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateKey(
    _In_ PHHIVE Hive,
    _In_ BOOLEAN SecurityDefaulted,
    _In_ HCELL_INDEX ParentCell,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG Flags,
    _In_ BOOLEAN FixHive)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    PCELL_DATA CellData;
    ULONG CellSize;
    BOOLEAN DoSubkeysRepair;
    ULONG TotalKeyNameLength, NameLength;

    PAGED_CODE();

    /* 当前键单元格不能为NIL */
    ASSERT(CurrentCell != HCELL_NIL);

    /* TODO: 一旦我们在Cm中支持安全缓存，就移除这个未使用的参数 */
    UNREFERENCED_PARAMETER(SecurityDefaulted);

    /*
     * 我们必须确保键单元格在开始之前已经分配。
     */
    if (!HvIsCellAllocated(Hive, CurrentCell))
    {
        DPRINT1("The key cell is not allocated\n");
        return CM_CHECK_REGISTRY_KEY_CELL_NOT_ALLOCATED;
    }

    /* 从单元格获取单元格数据 */
    CellData = reinterpret_cast<PCELL_DATA>(HvGetCell(Hive, CurrentCell));
    if (!CellData)
    {
        DPRINT1("Could not get cell data from the cell\n");
        return CM_CHECK_REGISTRY_CELL_DATA_NOT_FOUND;
    }

    /* 获取单元格的大小并验证其大小 */
    CellSize = HvGetCellSize(Hive, CellData);
    if (CellSize > CMP_KEY_SIZE_THRESHOLD)
    {
        DPRINT1("The cell size is above the threshold size (size %lu)\n", CellSize);
        return CM_CHECK_REGISTRY_CELL_SIZE_NOT_SANE;
    }

    /*
     * 单元格大小正常，但必须确保键不大于单元格本身。
     */
    NameLength = CellData->u.KeyNode.NameLength;
    if (NameLength == 0)
    {
        DPRINT1("The key node name length is 0!\n");
        return CM_CHECK_REGISTRY_KEY_NAME_LENGTH_ZERO;
    }

    TotalKeyNameLength = NameLength + FIELD_OFFSET(CM_KEY_NODE, Name);
    if (TotalKeyNameLength > CellSize)
    {
        DPRINT1("The key is too big than the cell (key size %lu, cell size %lu)\n", TotalKeyNameLength, CellSize);
        return CM_CHECK_REGISTRY_KEY_TOO_BIG_THAN_CELL;
    }

    /* 父单元格是否一致？ */
    if (ParentCell != HCELL_NIL &&
        ParentCell != CellData->u.KeyNode.Parent)
    {
        if (!CmpRepairParentNode(Hive,
                                 CurrentCell,
                                 ParentCell,
                                 CellData,
                                 FixHive))
        {
            DPRINT1("The parent key node doesn't point to the actual parent\n");
            return CM_CHECK_REGISTRY_BAD_KEY_NODE_PARENT;
        }
    }

    /* 键节点签名是否有效？ */
    if (CellData->u.KeyNode.Signature != CM_KEY_NODE_SIGNATURE)
    {
        if (!CmpRepairKeyNodeSignature(Hive,
                                       CurrentCell,
                                       CellData,
                                       FixHive))
        {
            DPRINT1("The parent key node signature is not valid\n");
            return CM_CHECK_REGISTRY_BAD_KEY_NODE_SIGNATURE;
        }
    }

    /*
     * FIXME: 安全单元格检查需要在这里实现，一旦我们在内核中正确可靠地实现安全缓存。
     */

    /* 验证类 */
    CmStatusCode = CmpValidateClass(Hive, CurrentCell, CellData);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        if (!CmpRepairClassOfNodeKey(Hive,
                                     CurrentCell,
                                     CellData,
                                     FixHive))
        {
            DPRINT1("Failed to repair the hive, the cell class is not valid\n");
            return CmStatusCode;
        }
    }

    /* 验证值列表 */
    CmStatusCode = CmpValidateValueList(Hive, CurrentCell, CellData, FixHive);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        /*
         * 如果列表中的某个值非常糟糕，例如我们无法从中映射单元格数据，或者列表本身完全损坏。
         * 在这种情况下，我们可以做的是“牺牲值列表”，即清除整个列表。
         */
        if (!CmpRepairValueList(Hive, CurrentCell, FixHive))
        {
            DPRINT1("Failed to repair the hive, the value list is corrupt\n");
            return CmStatusCode;
        }
    }

    /* 验证子键列表 */
    CmStatusCode = CmpValidateSubKeyList(Hive, CurrentCell, CellData, FixHive, reinterpret_cast<unsigned char*>(&DoSubkeysRepair));
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        /*
         * 子键列表出现问题。最糟糕的情况是，如果实际的子键列表严重损坏，这个键本身也会损坏。
         */
        if (!DoSubkeysRepair)
        {
            DPRINT1("The subkeys list is totally corrupt, can't repair\n");
            return CmStatusCode;
        }

        /*
         * 好吧，这个键还有一些救赎的机会。清除整个子键列表以修复它。
         */
        if (!CmpRepairSubKeyList(Hive,
                                 CurrentCell,
                                 CellData,
                                 FixHive))
        {
            DPRINT1("Failed to repair the hive, the subkeys list is corrupt!\n");
            return CmStatusCode;
        }
    }

    /* 如果需要，清除易失性数据 */
    CmpPurgeVolatiles(Hive, CurrentCell, CellData, Flags);
    return CM_CHECK_REGISTRY_GOOD;
}



/**
 * @brief
 * 通过使用基于堆栈的池，执行注册表的深度检查，遍历注册表树。
 * 此函数是CmCheckRegistry的核心。
 *
 * @param[in] Hive
 * 指向要执行验证的注册表蜂巢描述符的指针。
 *
 * @param[in] Flags
 * 用于易失性清除的位掩码标志。这些标志影响易失性清除的执行方式。详见CmCheckRegistry文档。
 *
 * @param[in] SecurityDefaulted
 * 如果调用者设置为FALSE，注册表蜂巢使用其独特的安全细节。
 * 否则，注册表蜂巢具有默认的安全细节。
 *
 * @param[in] FixHive
 * 如果设置为TRUE，目标蜂巢将被修复。
 *
 * @return
 * 如果函数成功执行了深度注册表检查且注册表内容有效，返回CM_CHECK_REGISTRY_GOOD。
 * 如果函数未能分配堆栈工作状态缓冲区，返回CM_CHECK_REGISTRY_ALLOCATE_MEM_STACK_FAIL。
 * 如果未找到该蜂巢的根单元格，返回CM_CHECK_REGISTRY_ROOT_CELL_NOT_FOUND。
 * 如果词法顺序无效，返回CM_CHECK_REGISTRY_BAD_LEXICOGRAPHICAL_ORDER。
 * 如果无法从键映射键节点，返回CM_CHECK_REGISTRY_NODE_NOT_FOUND。
 * 如果未找到子键子单元格，返回CM_CHECK_REGISTRY_SUBKEY_NOT_FOUND。
 * 如果我们达到最大堆栈限制，表示检查的注册表太庞大，返回CM_CHECK_REGISTRY_TREE_TOO_MANY_LEVELS。
 */
static
CM_CHECK_REGISTRY_STATUS
CmpValidateRegistryInternal(
    _In_ PHHIVE Hive,
    _In_ ULONG Flags,
    _In_ BOOLEAN SecurityDefaulted,
    _In_ BOOLEAN FixHive)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    PCMP_REGISTRY_STACK_WORK_STATE WorkState;
    HCELL_INDEX RootCell, ParentCell, CurrentCell;
    HCELL_INDEX ChildSubKeyCell;
    PCM_KEY_NODE KeyNode;
    ULONG WorkStateLength;
    LONG StackDepth;
    BOOLEAN AllChildrenChecked;

    PAGED_CODE();

    ASSERT(Hive);

    /*
     * 分配一些内存块用于堆栈状态结构。我们将使用它以递归方式遍历注册表蜂巢树，
     * 而不必担心会以最糟糕的方式耗尽内核堆栈。
     */
    WorkStateLength = CMP_REGISTRY_MAX_LEVELS_TREE_DEPTH * sizeof(CMP_REGISTRY_STACK_WORK_STATE);
    WorkState = static_cast<PCMP_REGISTRY_STACK_WORK_STATE>(CmpAllocate(WorkStateLength,
                            TRUE,
                            TAG_REGISTRY_STACK));
    if (!WorkState)
    {
        DPRINT1("Couldn't allocate memory for registry stack work state\n");
        return CM_CHECK_REGISTRY_ALLOCATE_MEM_STACK_FAIL;
    }

    /* 获取蜂巢的根单元格 */
    RootCell = GET_HHIVE_ROOT_CELL(Hive);
    if (RootCell == HCELL_NIL)
    {
        DPRINT1("Couldn't get the root cell of the hive\n");
        CmpFree(WorkState, WorkStateLength);
        return CM_CHECK_REGISTRY_ROOT_CELL_NOT_FOUND;
    }

RestartValidation:
    /*
     * 准备堆栈状态并从根单元格开始。确保根单元格本身正常后再继续。
     */
    StackDepth = 0;
    WorkState[StackDepth].ChildCellIndex = 0;
    WorkState[StackDepth].Current = RootCell;
    WorkState[StackDepth].Parent = HCELL_NIL;
    WorkState[StackDepth].Sibling = HCELL_NIL;

    /*
     * 由于我们开始检查根单元格，这是注册表蜂巢的顶层元素，
     * 我们将寻找子键，在遍历树的过程中。
     */
    AllChildrenChecked = FALSE;

    while (StackDepth >= 0)
    {
        /* 缓存当前和父单元格 */
        CurrentCell = WorkState[StackDepth].Current;
        ParentCell = WorkState[StackDepth].Parent;

        /* 我们是否还有子键需要验证？ */
        if (!AllChildrenChecked)
        {
            /* 检查键是否正常 */
            CmStatusCode = CmpValidateKey(Hive,
                                          SecurityDefaulted,
                                          ParentCell,
                                          CurrentCell,
                                          Flags,
                                          FixHive);
            if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
            {
                /*
                 * 键单元格损坏。我们必须祈祷并希望这不是根单元格，因为任何对根的损坏都是灾难性的。
                 */
                if (CurrentCell == RootCell)
                {
                    DPRINT1("THE ROOT CELL IS BROKEN\n");
                    CmpFree(WorkState, WorkStateLength);
                    return CmStatusCode;
                }

                /*
                 * 不是根单元格，从父单元格中移除损坏的单元格，以便修复蜂巢。
                 */
                if (!CmpRepairParentKey(Hive, CurrentCell, ParentCell, FixHive))
                {
                    DPRINT1("The key is corrupt (current cell %lu, parent cell %lu)\n",
                            CurrentCell, ParentCell);
                    CmpFree(WorkState, WorkStateLength);
                    return CmStatusCode;
                }

                /* 损坏的单元格已移除，重新启动循环 */
                DPRINT1("Hive repaired, restarting the validation loop...\n");
                goto RestartValidation;
            }

            /*
             * 键正常。如果我们已经推进了堆栈深度，则检查键的词法顺序。
             */
            if (StackDepth > 0 &&
                CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
            {
                if (WorkState[StackDepth - CMP_PRIOR_STACK].Sibling != HCELL_NIL)
                {
                    if (!CmpValidateLexicalOrder(Hive,
                                                 CurrentCell,
                                                 WorkState[StackDepth - CMP_PRIOR_STACK].Sibling))
                    {
                        /*
                         * 词法顺序无效，尝试修复蜂巢。
                         */
                        if (!CmpRepairParentKey(Hive, CurrentCell, ParentCell, FixHive))
                        {
                            DPRINT1("The lexicographical order is invalid (sibling %lu, current cell %lu)\n",
                            CurrentCell, WorkState[StackDepth - CMP_PRIOR_STACK].Sibling);
                            CmpFree(WorkState, WorkStateLength);
                            return CM_CHECK_REGISTRY_BAD_LEXICOGRAPHICAL_ORDER;
                        }

                        /* 损坏的单元格已移除，重新启动循环 */
                        DPRINT1("Hive repaired, restarting the validation loop...\n");
                        goto RestartValidation;
                    }
                }

                /* 为即将到来的迭代分配前一个兄弟 */
                WorkState[StackDepth - CMP_PRIOR_STACK].Sibling = CurrentCell;
            }
        }

        /* 获取该键的节点 */
        KeyNode = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, CurrentCell));
        if (!KeyNode)
        {
            DPRINT1("Couldn't get the node of key (current cell %lu)\n", CurrentCell);
            CmpFree(WorkState, WorkStateLength);
            return CM_CHECK_REGISTRY_NODE_NOT_FOUND;
        }

        /*
         * 如果我们已经处理了该节点的所有子键，则通过减少堆栈深度并告诉上面的代码我们已经检查了所有子键，
         * 以便我们不必再次验证相同的子键，而是继续下一个节点。
         */
        if (WorkState[StackDepth].ChildCellIndex < KeyNode->SubKeyCounts[Stable])
        {
            /*
             * 我们有子键要处理，获取相关的子键子单元格，以便我们可以在下一个键验证中缓存它。
             */
            ChildSubKeyCell = CmpFindSubKeyByNumber(Hive, KeyNode, WorkState[StackDepth].ChildCellIndex);
            if (ChildSubKeyCell == HCELL_NIL)
            {
                DPRINT1("Couldn't get the child subkey cell (at stack index %lu)\n", StackDepth);
                CmpFree(WorkState, WorkStateLength);
                return CM_CHECK_REGISTRY_SUBKEY_NOT_FOUND;
            }

            /*
             * 获取子键后，推进子键索引和堆栈深度工作状态以进行下一个键验证。
             * 但是，我们必须确保在推进堆栈深度时不超过最大树层深度。注册表树最多可以有512层深。
             * 更多信息请参见https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-element-size-limits。
             */
            WorkState[StackDepth].ChildCellIndex++;
            StackDepth++;
            if (StackDepth >= CMP_REGISTRY_MAX_LEVELS_TREE_DEPTH - 1)
            {
                /*
                 * 这个注册表树层数太多。我们不想耗尽内核堆栈，所以简单地退出...
                 */
                DPRINT1("The registry tree has so many levels!\n");
                CmpFree(WorkState, WorkStateLength);
                return CM_CHECK_REGISTRY_TREE_TOO_MANY_LEVELS;
            }

            /* 为下一个键准备工作状态 */
            WorkState[StackDepth].ChildCellIndex = 0;
            WorkState[StackDepth].Current = ChildSubKeyCell;
            WorkState[StackDepth].Parent = WorkState[StackDepth - CMP_PRIOR_STACK].Current;
            WorkState[StackDepth].Sibling = HCELL_NIL;

            /*
             * 准备工作状态后，确认循环顶部的代码路径我们需要处理并验证下一个子键子单元格。
             */
            AllChildrenChecked = FALSE;
            continue;
        }

        /*
         * 我们已经验证了该节点的所有子键子单元格。减少堆栈深度并告诉上面的代码我们已经检查了所有子键，
         * 以便我们不必再次验证相同的子键，而是继续下一个节点。
         */
        AllChildrenChecked = TRUE;
        StackDepth--;
        continue;
    }

    CmpFree(WorkState, WorkStateLength);
    return CM_CHECK_REGISTRY_GOOD;
}



/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * 验证一个蜂巢中的二进制文件。它对该二进制文件中的单元格进行检查，确保
 * 该二进制文件没有损坏并且单元格之间是一致的。
 *
 * @param[in] Hive
 * 指向要验证的蜂巢描述符的指针。
 *
 * @param[in] Bin
 * 指向要验证其单元格的二进制文件的指针。
 *
 * @return
 * 如果二进制文件有效且没有损坏，则返回CM_CHECK_REGISTRY_GOOD。
 * 如果该二进制文件的签名头损坏，则返回CM_CHECK_REGISTRY_BIN_SIGNATURE_HEADER_CORRUPT。
 * 如果自由单元格的大小无效，则返回CM_CHECK_REGISTRY_BAD_FREE_CELL。
 * 如果分配的单元格的大小无效，则返回CM_CHECK_REGISTRY_BAD_ALLOC_CELL。
 */
CM_CHECK_REGISTRY_STATUS

HvValidateBin(
    _In_ PHHIVE Hive,
    _In_ PHBIN Bin)
{
    PHCELL Cell, Basket;

    PAGED_CODE();

    ASSERT(Bin);
    ASSERT(Hive);

    /* 确保我们得到的这个二进制文件具有有效的签名头 */
    if (Bin->Signature != HV_HBIN_SIGNATURE)
    {
        DPRINT1("The bin's signature header is corrupt\n");
        return CM_CHECK_REGISTRY_BIN_SIGNATURE_HEADER_CORRUPT;
    }

    /*
     * 遍历该二进制文件中的所有单元格，并验证它们与二进制文件的一致性。
     * 具体来说，我们希望每个单元格的大小都有效。
     */
    Basket = (PHCELL)((unsigned char*)Bin + Bin->Size);
    for (Cell = GET_CELL_BIN(Bin);
         Cell < Basket;
         Cell = (PHCELL)((unsigned char*)Cell + abs(Cell->Size)))
    {
        if (IsFreeCell(Cell))
        {
            /*
             * 这个单元格是自由的，检查该单元格的大小是否有效。
             */
            if (Cell->Size > Bin->Size ||
                Cell->Size == 0)
            {
                /*
                 * 这个单元格的自由空间超过了二进制文件的大小。
                 * 否则，单元格没有实际的自由空间（即大小为0），这是不允许的。
                 */
                DPRINT1("The free cell exceeds the bin size or cell size equal to 0 (cell 0x%p, cell size %d, bin size %lu)\n",
                        Cell, Cell->Size, Bin->Size);
                return CM_CHECK_REGISTRY_BAD_FREE_CELL;
            }
        }
        else
        {
            /*
             * 这个单元格是分配的，确保该单元格的大小有效。
             */
            if (abs(Cell->Size) > Bin->Size)
            {
                /*
                 * 这个单元格分配的空间超过了二进制文件的大小。
                 */
                DPRINT1("The allocated cell exceeds the bin size (cell 0x%p, cell size %d, bin size %lu)\n",
                        Cell, abs(Cell->Size), Bin->Size);
                return CM_CHECK_REGISTRY_BAD_ALLOC_CELL;
            }
        }
    }

    return CM_CHECK_REGISTRY_GOOD;
}


/**
 * @brief
 * 验证一个注册表蜂巢。此函数确保该蜂巢的存储具有有效的二进制文件。
 *
 * @param[in] Hive
 * 指向要验证其蜂巢二进制文件的蜂巢描述符的指针。
 *
 * @return
 * 如果蜂巢有效，则返回CM_CHECK_REGISTRY_GOOD。
 * 如果蜂巢的签名损坏，则返回CM_CHECK_REGISTRY_HIVE_CORRUPT_SIGNATURE。
 * 如果捕获的二进制文件的大小或偏移量无效，则返回CM_CHECK_REGISTRY_BIN_SIZE_OR_OFFSET_CORRUPT。
 * 否则返回失败的CM状态码。
 */
CM_CHECK_REGISTRY_STATUS

HvValidateHive(
    _In_ PHHIVE Hive)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    ULONG StorageIndex;
    ULONG BlockIndex;
    ULONG StorageLength;
    PHBIN Bin;

    PAGED_CODE();

    ASSERT(Hive);

    /* 蜂巢的签名是否有效？ */
    if (Hive->Signature != HV_HHIVE_SIGNATURE)
    {
        DPRINT1("Hive's signature corrupted (signature %lu)\n", Hive->Signature);
        return CM_CHECK_REGISTRY_HIVE_CORRUPT_SIGNATURE;
    }

    /*
     * 现在遍历该蜂巢存储中的每个二进制文件。
     */
    for (StorageIndex = 0; StorageIndex < Hive->StorageTypeCount; StorageIndex++)
    {
        /* 获取该索引处的存储长度 */
        StorageLength = Hive->Storage[StorageIndex].Length;

        for (BlockIndex = 0; BlockIndex < StorageLength;)
        {
            /* 如果该二进制文件不存在，则继续下一个 */
            if (Hive->Storage[StorageIndex].BlockList[BlockIndex].BinAddress == (ULONG_PTR)NULL)
            {
                continue;
            }

            /*
             * 捕获该二进制文件，并确保其偏移量和大小有效。
             */
            Bin = GET_HHIVE_BIN(Hive, StorageIndex, BlockIndex);
            if (Bin->Size > (StorageLength * HBLOCK_SIZE) ||
                (Bin->FileOffset / HBLOCK_SIZE) != BlockIndex)
            {
                DPRINT1("Bin size or offset is corrupt (bin size %lu, file offset %lu, storage length %lu)\n",
                        Bin->Size, Bin->FileOffset, StorageLength);
                return CM_CHECK_REGISTRY_BIN_SIZE_OR_OFFSET_CORRUPT;
            }

            /* 验证该二进制文件的其余部分 */
            CmStatusCode = HvValidateBin(Hive, Bin);
            if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
            {
                DPRINT1("This bin is not valid (bin 0x%p)\n", Bin);
                return CmStatusCode;
            }

            /* 继续下一个块 */
            BlockIndex += Bin->Size / HBLOCK_SIZE;
        }
    }

    return CM_CHECK_REGISTRY_GOOD;
}


/**
 * @brief
 * 检查注册表是否一致且其内容有效且未损坏。更具体地说，此函数对注册表进行深度检查，
 * 以确保以下属性：
 *
 * - 注册表的安全缓存单元格正常
 * - 二进制文件和单元格之间一致
 * - 子键单元格指向父键
 * - 键本身具有合理的大小
 * - 类、值和子键列表有效
 * - 更多
 *
 * @param[in] Hive
 * 指向要检查的CM蜂巢的指针。
 *
 * @param[in] Flags
 * 一个位掩码标志，用于影响易失性键清除的过程。详见备注。
 *
 * @return
 * 此函数返回一个CM（配置管理器）检查注册表状态码。值为0的CM_CHECK_REGISTRY_GOOD表示
 * 注册表蜂巢有效且未损坏。非零的无符号整数值表示失败。请查阅此文件中的其他私有例程以
 * 获取其他失败状态码。
 *
 * @remarks
 * 在加载操作期间，CmCheckRegistry可以根据调用者提交的标志位掩码清除注册表的易失性数据
 * （或不）。支持的标志如下：
 *
 * CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES -- 告诉函数不要清除易失性数据。
 *
 * CM_CHECK_REGISTRY_PURGE_VOLATILES - 告诉函数清除注册表蜂巢中的易失性信息数据，如果
 * 发现易失性数据。
 *
 * CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES - 由FreeLdr和Environ使用的特殊标志。
 * 当设置此标志时，函数不会清理易失性存储，而是会取消初始化存储（如果给定的注册表蜂巢
 * 是XP Beta 1蜂巢或更新版本）。否则，它会执行易失性存储的正常清理。
 *
 * CM_CHECK_REGISTRY_VALIDATE_HIVE - 告诉函数在对注册表树进行验证之前，对底层蜂巢的二进制
 * 文件和单元格进行彻底分析。在这种情况下会调用HvValidateHive函数。
 *
 * CM_CHECK_REGISTRY_FIX_HIVE - 告诉函数如果目标注册表蜂巢损坏，则修复它。通常此标志来自
 * 注册表修复工具，用户要求修复其损坏的蜂巢。在这种情况下，会对蜂巢进行自愈过程。
 */
CM_CHECK_REGISTRY_STATUS

CmCheckRegistry(
    _In_ PCMHIVE RegistryHive,
    _In_ ULONG Flags)
{
    CM_CHECK_REGISTRY_STATUS CmStatusCode;
    PHHIVE Hive;
    BOOLEAN ShouldFixHive = FALSE;

    PAGED_CODE();

    /* 如果调用者没有提供蜂巢，则退出 */
    if (!RegistryHive)
    {
        DPRINT1("No registry hive given for check\n");
        return CM_CHECK_REGISTRY_INVALID_PARAMETER;
    }

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    /*
     * 主蜂巢是注册表的根，它将所有其他蜂巢连接在一起。因此不进行任何验证检查。
     */
    if (RegistryHive == CmiVolatileHive)
    {
        DPRINT("This is master registry hive, don't do anything\n");
        return CM_CHECK_REGISTRY_GOOD;
    }
#endif

    /* 如果没有提供有效的标志，则退出 */
    if (Flags & ~(CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES       |
                  CM_CHECK_REGISTRY_PURGE_VOLATILES            |
                  CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES |
                  CM_CHECK_REGISTRY_VALIDATE_HIVE              |
                  CM_CHECK_REGISTRY_FIX_HIVE))
    {
        DPRINT1("Invalid flag for registry check given (flag %lu)\n", Flags);
        return CM_CHECK_REGISTRY_INVALID_PARAMETER;
    }

    /*
     * 获取蜂巢并检查调用者是否希望验证蜂巢。
     */
    Hive = GET_HHIVE(RegistryHive);
    if (Flags & CM_CHECK_REGISTRY_VALIDATE_HIVE)
    {
        CmStatusCode = HvValidateHive(Hive);
        if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
        {
            DPRINT1("The hive is not valid (hive 0x%p, check status code %lu)\n", Hive, CmStatusCode);
            return CmStatusCode;
        }
    }

    /*
     * 如果注册表修复工具（如ReactOS Check Registry Utility）希望在检查目标蜂巢时修复损坏的蜂巢。
     */
    if (Flags & CM_CHECK_REGISTRY_FIX_HIVE)
    {
        ShouldFixHive = TRUE;
    }

    /*
     * FIXME: 目前ReactOS没有实现安全缓存算法，因此现在实现安全描述符验证检查是没有意义的。
     * 当需要实现这些时，我们需要在这里实现安全检查。
     */

    /* 调用内部API来完成其余的工作 */
    CmStatusCode = CmpValidateRegistryInternal(Hive, Flags, FALSE, ShouldFixHive);
    if (!CM_CHECK_REGISTRY_SUCCESS(CmStatusCode))
    {
        DPRINT1("The hive is not valid (hive 0x%p, check status code %lu)\n", Hive, CmStatusCode);
        return CmStatusCode;
    }

    return CmStatusCode;
}


/* EOF */
