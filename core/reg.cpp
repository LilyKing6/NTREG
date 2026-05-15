/*
 * PROJECT:   Registry Library
 * FILE:      reg.cpp
 * PURPOSE:   Glue file — pulls in the split modules
 *
 * The registry implementation is split across:
 *   reg_globals.cpp  — global state and shared helpers
 *   reg_path.cpp     — path walking engine (RegpCreateOrOpenKey)
 *   reg_keyops.cpp   — key operations (open, close, create, delete, enum)
 *   reg_valops.cpp   — value operations (query, set, delete, enum)
 *   reg_hive_mgmt.cpp — hive lifecycle (connect, load, init, shutdown, InitNTReg)
 */

/* BitScan intrinsics for non-MSVC 64-bit */
#ifdef _MSC_VER
    #include <intrin.h>
#else
    #include <stdint.h>
    int _BitScanForward64(unsigned long* index, uint64_t mask)
    {
        if (mask == 0) return 0;
        *index = __builtin_ctzll(mask);
        return 1;
    }
    int _BitScanReverse64(unsigned long* index, uint64_t mask)
    {
        if (mask == 0) return 0;
        *index = 63 - __builtin_clzll(mask);
        return 1;
    }
#endif
