# AGENTS.md

NTREG registry-like key-value tree database. Reuses Windows registry hive binary format and internal data structures, runs as a standalone database without Windows kernel.

## Build

```bash
cmake -B build && cmake --build build -j$(nproc)
```

**Prerequisites:** GCC (tested with g++ 14) or Clang, CMake >= 3.20, C++20 support.

Hive files must exist at `build/Config/{SYSTEM,SOFTWARE,DEFAULT}` before running anything. Generate them:

```bash
cd build
mkdir -p Config
./mkhive -h:SYSTEM -u -d:Config ../reginit/hivesys.inf
./mkhive -h:SOFTWARE -u -d:Config ../reginit/hivesft.inf
./mkhive -h:DEFAULT -u -d:Config ../reginit/hivedef.inf
```

## Test

**Integration:** `build/test_api` — 13 cases covering create/open/delete keys, set/get string/dword/binary, enumerate keys/values, subkey operations.

**Unit tests (8 executables):**

```bash
cd build/tests/unit
ln -sf ../../Config Config          # hive files must be reachable
for t in cm_*_tests; do ./$t; done
```

| Test | Suite | Cases | Purpose |
|---|---|---|---|
| `cm_api_tests` | API | 6 | Initialize, open/create/delete keys, move semantics |
| `cm_core_tests` | Core | 6 | Native handles, enumeration, subkeys |
| `cm_value_tests` | ValueBasic | 8 | Dword/string/binary round-trip, empty, delete |
| `cm_value_adv_tests` | Value | 9 | Multi-string, qword, overwrite types, large data |
| `cm_index_tests` | Index | 4 | Many subkeys, by-name lookup, Unicode names |
| `cm_check_tests` | Check | 5 | Consistency, error handling, empty enum |
| `cm_hive_tests` | Hive | 4 | SYSTEM access, multi-hive init, cross-hive create |
| `cm_path_tests` | Path | 4 | Deep nesting, escapes, root traversal, missing keys |

Test runner is a header-only Criterion shim (`tests/unit/criterion_shim.hpp`). No external test framework dependency.

## Architecture

```
\NTReg (root)
├── \NTReg\Local                           HKEY_LOCAL_MACHINE equivalent
│   ├── SYSTEM                             main system configuration hive
│   ├── SOFTWARE                           software configuration hive
│   ├── HARDWARE, SAM, SECURITY            optional additional hives
│   └── TEMP                               volatile scratch space
└── \NTReg\User                            HKEY_USERS equivalent
    └── .Default                           default user profile
```

### Layers

1. **Binary format layer** — Windows registry hive format on disk
   - `HBASE_BLOCK` (4096 bytes), `HBIN`, cells, bins
   - Files: `core/hiveinit.cpp`, `core/hivebin.cpp`, `core/hivecell.cpp`, `core/hivewrt.cpp`, `core/hivesum.cpp`

2. **Cell-level API** — allocate/free/read/write cells
   - `HvAllocateCell()`, `HvFreeCell()`, `HvGetCell()`, `HvReleaseCell()`
   - File: `core/bitmap.cpp`, headers: `include/hivedata.hpp`, `include/cmdata.hpp`

3. **Key/value index** — find keys by name, add/remove subkeys, manage value lists
   - `CmpFindSubKeyByName()`, `CmpAddSubKey()`, `CmpFindValueByName()`, `CmpSetValueDataNew()`
   - Files: `core/cmindex.cpp`, `core/cmname.cpp`, `core/cmvalue.cpp`, `core/cmi.cpp`

4. **NTReg vtable API** — Windows-compatible `NTReg*` interface
   - `RegInitializeRegistry()`, `NTRegCreateKeyW()`, `NTRegSetValueExW()`, etc.
   - `struct NTReg` with function pointers (OpenKey, CloseKey, CreateKey, etc.)
   - Files: `core/reg_hive_mgmt.cpp`, `core/reg_keyops.cpp`, `core/reg_valops.cpp`, `core/reg_path.cpp`, `core/reg_globals.cpp`

5. **Modern C++20 API** — `registry::Key`, `registry::Registry`, `registry::HiveInitializer`
   - RAII handles, `std::optional` returns, exceptions
   - Files: `core/registry_api.cpp`, `core/hive_init.cpp`
   - Headers: `include/registry_api.hpp`, `include/hive_init.hpp`, `include/exceptions.hpp`, `include/string.hpp`, `include/types.hpp`

6. **Validation/repair** — `CmCheckRegistry()`, self-healing
   - Files: `core/cmcheck.cpp`, `core/cmheal.cpp`

### Key headers

| Header | Purpose |
|---|---|
| `include/typedefs.hpp` | `WCHAR`, `PWCHAR`, `UNICODE_STRING`, platform-neutral types |
| `include/types.hpp` | `registry::u8/u16/u32/u64`, `ValueType`, `HCELL_INDEX` |
| `include/cmlib.hpp` | Core definitions, status codes, function declarations |
| `include/hivedata.hpp` | On-disk format structures (`HBASE_BLOCK`, `HBIN`, `HHIVE`) |
| `include/cmdata.hpp` | Cell structures (`CM_KEY_NODE`, `CM_KEY_VALUE`, `CHILD_LIST`) |
| `include/reg.hpp` | Public API, error codes, `DIR_SEPARATOR_CHAR` |
| `include/reg_internal.hpp` | Shared internal types (`MEMKEY`, `REPARSE_POINT`, global state) |
| `include/unicode.hpp` | `strlenW`, `strcmpiW`, character classification |

## WCHAR = char16_t (Linux compatibility)

On Linux `wchar_t` is 4 bytes, but the registry binary format uses 2-byte characters. `include/typedefs.hpp:80` defines:

```cpp
using WCHAR = char16_t;  // guaranteed 2 bytes on all platforms
```

This propagates through `PWCHAR`, `PCWSTR`, `UNICODE_STRING`, and all core code. The public C++ API uses `std::u16string` / `std::u16string_view`. String literals use `u"..."` prefix.

Helper functions in `include/types.hpp`:
- `_wcs_len_char16(const char16_t*)` — length (replaces `std::wcslen`)
- `_wcs_cmp_char16(const char16_t*, const char16_t*)` — comparison (replaces `std::wcscmp`)
- `_to_upper_char16(char16_t)` / `_to_lower_char16(char16_t)` — ASCII case conversion

### Display of char16_t

`std::cout` does not accept `char16_t` strings. Utility programs that need display output must convert to narrow strings:

```cpp
std::string u16_to_narrow(const std::u16string& s) {
    std::string result; result.reserve(s.size());
    for (char16_t c : s) result.push_back(static_cast<char>(c & 0x7F));
    return result;
}
```

## Known issues

1. **Shutdown lifecycle** — multi-test suites skip `RegShutdownRegistry` in `.fini` to avoid potential issues. `Registry::shutdown` frees `g_reg` after `RegShutdownRegistry`, but hive objects may already be freed internally. The `init`/`shutdown`/`init` cycle works correctly in standalone tests, but the test framework skips shutdown as a precaution.

2. **Cell free list corruption (mitigated)** — `HvFreeCell` coalescing could produce stale free list entries with incorrect `Size` fields, leading to out-of-bounds writes during `HvAllocateCell` cell splitting. Fixed by adding bin-boundary validation in `HvpFindFree`, `HvAllocateCell`, and `HvFreeCell` (pointer-based bounds checks, corrupted entry skipping, and zero-size cell guards). Verified with ASAN.

## File map

```
reg/
├── core/                          # 28 source files (static library `libreg.a`)
├── include/                       # 21 headers
│   └── compat/                    # pshpack1.h, poppack.h (pragma pack compat)
├── tools/                         # Executables
│   ├── mkhive.cpp + reginf.cpp    # hive file generator from INF
│   ├── test_api.cpp               # integration test (modern C++ API)
│   ├── regexplorer.cpp            # interactive browser (modern C++ API)
│   ├── test_fileload.cpp          # disk load test (NTReg API)
│   └── inithive_modern.cpp        # hive init with version info
├── tests/unit/                    # 8 test sources + criterion_shim.hpp
├── reginit/                       # INF files for hive creation
└── CMakeLists.txt                 # top-level build config
```
