/*
 * PROJECT:   注册表操作库
 * LICENSE:   See LICENSE in the top level directory
 * COPYRIGHT: Copyright 2024 Lily King
 */

#include "cmlib.hpp"

/**
 * @brief 计算蜂巢头校验和
 *
 * 这个函数用于计算蜂巢头的校验和并返回。
 *
 * @param HiveHeader 蜂巢头指针
 * @return 计算得到的校验和
 */
ULONG CMAPI
HvpHiveHeaderChecksum(
    PHBASE_BLOCK HiveHeader)
{
    PULONG Buffer = (PULONG)HiveHeader;
    ULONG Sum = 0;
    ULONG i;

    // 计算校验和
    for (i = 0; i < 127; i++)
        Sum ^= Buffer[i];
    if (Sum == (ULONG)-1)
        Sum = (ULONG)-2;
    if (Sum == 0)
        Sum = 1;

    return Sum;
}

