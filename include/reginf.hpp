/*
 *  NTREG kernel
 *  Copyright (C) 2024 NTSoft
 *
 * COPYRIGHT:       See LICENSE in the top level directory
 * PROJECT:         NTREG hive maker
 * FILE:            reginf.h
 * PURPOSE:         Inf file import code
 * PROGRAMMER:      Lily King
 */

#pragma once

#include "typedefs.hpp"
#include "cmlib.hpp"
#include "cmi.hpp"
#include "reg.hpp"
#include "infhost.hpp"

BOOL
ImportRegistryFile(char* Filename);

/* EOF */
