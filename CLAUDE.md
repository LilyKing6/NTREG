# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NTREG is a Windows Registry hive implementation library that provides low-level registry operations. The codebase implements the Windows Registry hive format, allowing creation, reading, and manipulation of registry hives outside of Windows kernel context.

## Build Commands

**Build the project:**
```bash
bd.cmd
```
This creates a `build/` directory, runs CMake with MinGW Makefiles generator, and compiles with 8 parallel jobs.

**Manual build:**
```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j 8
```

**Clean build artifacts:**
```bash
clean.cmd
```

## Directory Structure

```
reg/
├── core/           # Core library source files (.cpp)
├── include/        # All header files (.hpp/.h)
├── tools/          # Executable programs (mkhive, inithive, regexplorer, test_api)
├── reginit/        # INF initialization files for hive creation
├── build/          # Build output (gitignored)
├── CMakeLists.txt  # Build configuration
├── bd.cmd          # Build script
└── clean.cmd       # Clean script
```

## Architecture

### Core Components

**Hive Management (cmlib.hpp, hivedata.hpp)**
- `CMHIVE`: Main hive structure containing the `HHIVE` base and file handles
- `HHIVE`: Low-level hive structure with storage management
- Hive operations: initialization, cell allocation/deallocation, synchronization

**Cell Management (core/hivecell.cpp, core/hivebin.cpp)**
- Registry data stored in cells within bins
- `HCELL_INDEX`: 32-bit cell offset identifier
- Cell operations: `HvAllocateCell()`, `HvFreeCell()`, `HvGetCell()`, `HvReleaseCell()`

**Key/Value Operations (core/cmindex.cpp, core/cmvalue.cpp, core/cmname.cpp)**
- `CM_KEY_NODE`: Registry key structure
- `CM_KEY_VALUE`: Registry value structure
- Key lookup: `CmpFindSubKeyByName()`, `CmpFindSubKeyByNumber()`
- Value operations: `CmpFindValueByName()`, `CmpValueToData()`

**Registry API (core/reg.cpp, include/reg.hpp)**
- High-level registry API similar to Windows RegOpenKey/RegSetValue
- `NTReg` structure provides function pointers for registry operations
- Predefined hives: SYSTEM, SOFTWARE, SAM, SECURITY, HARDWARE

**Hive Validation & Repair (core/cmcheck.cpp, core/cmheal.cpp)**
- `CmCheckRegistry()`: Validates hive integrity
- `HvValidateBin()`, `HvValidateHive()`: Low-level validation
- Self-healing routines repair corrupted structures when possible

### Key Data Structures

**Registry Hive Hierarchy:**
```
\NTReg (root)
├── \NTReg\Local (HKEY_LOCAL_MACHINE equivalent)
│   ├── HARDWARE
│   ├── SYSTEM
│   ├── SOFTWARE
│   ├── SAM
│   └── SECURITY
└── \NTReg\User (HKEY_USERS equivalent)
    └── .Default
```

**Memory Layout:**
- Hives consist of bins (4KB aligned blocks)
- Bins contain cells (variable-sized data blocks)
- Cells store keys, values, security descriptors, and index structures

### Important Header Files

All headers are in `include/`:
- **cmlib.hpp**: Core definitions, status codes, function declarations
- **hivedata.hpp**: Hive on-disk format structures
- **reg.hpp**: Public API definitions and error codes
- **typedefs.hpp**: Platform-independent type definitions
- **unicode.hpp**: Unicode string handling

## Development Notes

**Platform Compatibility:**
- Cross-platform (Windows/Unix) with conditional compilation
- Uses `DIR_SEPARATOR_CHAR` ('\\' on Windows, '/' on Unix)
- MSVC and GCC support

**Compiler Flags:**
- `-O3`: Optimize for speed
- `-s`: Strip symbols
- `-static`: Static linking
- Optional: `-DNASSERT` to disable assertions

**Registry Types:**
- `REG_SZ` (1): String
- `REG_BINARY` (3): Binary data
- `REG_DWORD` (4): 32-bit integer
- `REG_MULTI_SZ` (7): Multi-string
- `REG_QWORD` (11): 64-bit integer

**Error Codes:**
- `ERROR_SUCCESS` (0): Operation succeeded
- `ERROR_FILE_NOT_FOUND` (2): Key/value not found
- `ERROR_ACCESS_DENIED` (5): Permission denied
- `ERROR_INVALID_PARAMETER` (87): Invalid parameter
- `ERROR_NO_MORE_ITEMS` (259): Enumeration complete

## Modern C++20 API

The project includes modern C++20 wrappers in `include/`:

- **types.hpp**: `u8/u16/u32/u64`, `i8/i16/i32/i64`, `enum class StorageType/FileType/ValueType`
- **string.hpp**: `RegistryString` class replacing `UNICODE_STRING`
- **exceptions.hpp**: `RegistryException`, `KeyNotFoundException`, `AccessDeniedException`
- **exceptions.hpp**: `RegistryException`, `KeyNotFoundException`, `AccessDeniedException`
- **registry_api.hpp + core/registry_api.cpp**: Modern `Key` and `Registry` classes with RAII
- **hive_init.hpp + core/hive_init.cpp**: `HiveInitializer` with batch INF→Hive processing

## Testing

**Integration test (test_api.exe):**
```bash
cd build && ./test_api.exe
```
Tests the modern C++ API: create/open/delete keys, set/get string/dword/binary values, enumerate keys/values, subkey operations. 13 test cases.

**Hive file loading test (test_fileload.exe):**
```bash
cd build && mkdir -p Config && cp hiveload_test/system Config/system && ./test_fileload.exe
```
Tests HINIT_FILE loading path (block-by-block from disk).

**Hive creation test (mkhive.exe):**
```bash
cd build && ./mkhive.exe -h:SYSTEM,SOFTWARE,DEFAULT -d:output reginit/hivesys.inf reginit/hivesft.inf reginit/hivedef.inf
```

## Completed Work

1. ~~**Implement HiveInitializer**~~ — `core/hive_init.cpp` with batch processing.
2. ~~**Integration test modern API**~~ — `tools/test_api.cpp`, 13/13 tests passing.
3. ~~**Fix Registry::shutdown double-shutdown bug**~~ — Removed duplicate `RegShutdownRegistry()` call, added `free(g_reg)`.
4. ~~**Fix core FIXMEs Phase 1**~~ — cmi.cpp error path leaks, bitmap.cpp ASSERT, HvReallocateCell shrinking, DestKey validation.
5. ~~**Fix core FIXMEs Phase 2**~~ — KEY_PREDEF_HANDLE handling, big value validation in cmcheck.cpp, stale comments cleanup.
6. ~~**Fix hiveinit.cpp FIXMEs Phase 3**~~ — LogSize clearing, sequence mismatch detection, RecoverData path implementation.
7. ~~**Fix hiveinit.cpp monolithic read Phase 4**~~ — Replaced with `HvpInitializeFileHive`: block-by-block bin loading from disk.
8. ~~**Modernize regexplorer**~~ — Rewritten using modern `registry::Key`/`Registry` API. Fixed memory leak and string conversion bugs.
9. ~~**Fix error handling**~~ — `registry_api.cpp` now maps NTReg return codes correctly (KeyNotFound/AccessDenied/InvalidParameter/OutOfMemory). `load_hive()` uses `hive_path` parameter.
10. ~~**Clean up dead code**~~ — Deleted `include/memory.hpp` (never included).
11. ~~**Optimize project architecture**~~ — Extracted security descriptors to `security_descriptors.cpp`. Fixed `rtl.cpp #include "bitmap.cpp"` hack (now standalone TU). Split `reg.cpp` (1513 lines) into 5 focused modules (`reg_globals`, `reg_path`, `reg_keyops`, `reg_valops`, `reg_hive_mgmt`). Created `reg_internal.hpp` for shared internal types. Fixed `HivePath` extern hack in `hive_init.cpp`.

## Pending Tasks

1. **Git 初始化 + 发布准备** — 推送到 GitHub，设置 .gitignore、README、CI.
