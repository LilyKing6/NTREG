/*
 * PROJECT:         NTREG Kernel
 * LICENSE:         See LICENSE in the top level directory
 * FILE:            cmlib/cmse.c
 * PURPOSE:         Configuration Manager Library - Security Subsystem Interface
 * PROGRAMMERS:     Lily King
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include "cmlib.hpp"

#include "debug.hpp"

/* FUNCTIONS *****************************************************************/

/**
 * @brief 移除安全单元格列表
 *
 * 这个函数用于从蜂巢中移除一个安全单元格列表。
 *
 * @param Hive 蜂巢指针
 * @param SecurityCell 安全单元格索引
 */
void
CmpRemoveSecurityCellList(IN PHHIVE Hive,
                          IN HCELL_INDEX SecurityCell)
{
    PCM_KEY_SECURITY SecurityData, FlinkCell, BlinkCell;

    PAGED_CODE();

    // 获取安全数据单元格
    SecurityData = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(Hive, SecurityCell));
    if (!SecurityData) return;

    // 获取前向链接单元格
    FlinkCell = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(Hive, SecurityData->Flink));
    if (!FlinkCell)
    {
        HvReleaseCell(Hive, SecurityCell);
        return;
    }

    // 获取后向链接单元格
    BlinkCell = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(Hive, SecurityData->Blink));
    if (!BlinkCell)
    {
        HvReleaseCell(Hive, SecurityData->Flink);
        HvReleaseCell(Hive, SecurityCell);
        return;
    }

    // 检查链接完整性
    ASSERT(FlinkCell->Blink == SecurityCell);
    ASSERT(BlinkCell->Flink == SecurityCell);

    // 解除安全块的链接并释放
    FlinkCell->Blink = SecurityData->Blink;
    BlinkCell->Flink = SecurityData->Flink;
#ifdef USE_CM_CACHE
    CmpRemoveFromSecurityCache(Hive, SecurityCell);
#endif

    // 释放单元格
    HvReleaseCell(Hive, SecurityData->Blink);
    HvReleaseCell(Hive, SecurityData->Flink);
    HvReleaseCell(Hive, SecurityCell);
}


/**
 * @brief 释放安全描述符
 *
 * 这个函数用于释放蜂巢中的安全描述符。
 *
 * @param Hive 蜂巢指针
 * @param Cell 单元格索引
 */
void
CmpFreeSecurityDescriptor(IN PHHIVE Hive,
                          IN HCELL_INDEX Cell)
{
    PCM_KEY_NODE CellData;
    PCM_KEY_SECURITY SecurityData;

    PAGED_CODE();

    // 获取单元格数据
    CellData = reinterpret_cast<PCM_KEY_NODE>(HvGetCell(Hive, Cell));
    if (!CellData) return;

    // 检查单元格签名
    ASSERT(CellData->Signature == CM_KEY_NODE_SIGNATURE);

    // 检查单元格是否有安全块
    if (CellData->Security == HCELL_NIL)
    {
        DPRINT("Cell 0x%08x (data 0x%p) has no security block!\n", Cell, CellData);
        HvReleaseCell(Hive, Cell);
        return;
    }

    // 获取安全数据
    SecurityData = reinterpret_cast<PCM_KEY_SECURITY>(HvGetCell(Hive, CellData->Security));
    if (!SecurityData)
    {
        HvReleaseCell(Hive, Cell);
        return;
    }

    // 检查安全数据签名
    ASSERT(SecurityData->Signature == CM_KEY_SECURITY_SIGNATURE);

    // 减少引用计数或移除安全块
    if (SecurityData->ReferenceCount > 1)
    {
        SecurityData->ReferenceCount--;
    }
    else // if (SecurityData->ReferenceCount <= 1)
    {
        CmpRemoveSecurityCellList(Hive, CellData->Security);
        HvFreeCell(Hive, CellData->Security);
    }

    // 清除单元格的安全索引
    CellData->Security = HCELL_NIL;
    HvReleaseCell(Hive, CellData->Security);
    HvReleaseCell(Hive, Cell);
}

