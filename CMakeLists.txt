cmake_minimum_required(VERSION 3.20)
project(RegistryLib CXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Unit tests infrastructure
add_library(reg_unit_tests INTERFACE)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(REG_SOURCES
    core/binhive.cpp
    core/bitmap.cpp
    core/cmcheck.cpp
    core/cmheal.cpp
    core/cmi.cpp
    core/cmindex.cpp
    core/cminit.cpp
    core/cmkeydel.cpp
    core/cmname.cpp
    core/cmse.cpp
    core/cmvalue.cpp
    core/hivebin.cpp
    core/hivecell.cpp
    core/hive_init.cpp
    core/hiveinit.cpp
    core/hivesum.cpp
    core/hivewrt.cpp
    core/reg.cpp
    core/reg_globals.cpp
    core/reg_path.cpp
    core/reg_keyops.cpp
    core/reg_valops.cpp
    core/reg_hive_mgmt.cpp
    core/registry_api.cpp
    core/rtl.cpp
    core/Pal.cpp
    core/security_descriptors.cpp
)

set(INITHIVE_SOURCE tools/inithive_modern.cpp)
set(MKHIVE_SOURCE tools/mkhive.cpp tools/reginf.cpp)
set(REGEXPLORER_SOURCE tools/regexplorer.cpp)
set(TEST_API_SOURCE tools/test_api.cpp)
set(TEST_FILELOAD_SOURCE tools/test_fileload.cpp)

add_library(reg STATIC ${REG_SOURCES})
target_include_directories(reg PUBLIC include/ include/compat/)

# find_package(Criterion REQUIRED)  -- using header-only shim
# find_package(Threads REQUIRED)    -- not needed with shim

# Prepare reg_unit_tests target for test suites
# target_link_libraries removed -- using shim

# Performance optimization flags
if(MSVC)
    target_compile_options(reg PRIVATE /O2 /GL /W4)
    target_link_options(reg PRIVATE /LTCG)
else()
    target_compile_options(reg PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(reg PRIVATE  )
endif()

add_executable(inithive ${INITHIVE_SOURCE})
target_link_libraries(inithive reg)

add_executable(mkhive ${MKHIVE_SOURCE})
target_link_libraries(mkhive reg)

add_executable(regexplorer ${REGEXPLORER_SOURCE})
target_link_libraries(regexplorer reg)

# Register tests/unit subdirectory so unit tests are available
add_subdirectory(tests/unit)

add_executable(test_api ${TEST_API_SOURCE})
target_link_libraries(test_api reg)

add_executable(test_fileload ${TEST_FILELOAD_SOURCE})
target_link_libraries(test_fileload reg)

# Apply same optimizations to executables
if(MSVC)
    target_compile_options(inithive PRIVATE /O2 /GL /W4)
    target_link_options(inithive PRIVATE /LTCG)
    target_compile_options(mkhive PRIVATE /O2 /GL /W4)
    target_link_options(mkhive PRIVATE /LTCG)
    target_compile_options(regexplorer PRIVATE /O2 /GL /W4)
    target_link_options(regexplorer PRIVATE /LTCG)
    target_compile_options(test_api PRIVATE /O2 /GL /W4)
    target_link_options(test_api PRIVATE /LTCG)
    target_compile_options(test_fileload PRIVATE /O2 /GL /W4)
    target_link_options(test_fileload PRIVATE /LTCG)
else()
    target_compile_options(inithive PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(inithive PRIVATE  )
    target_compile_options(mkhive PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(mkhive PRIVATE  )
    target_compile_options(regexplorer PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(regexplorer PRIVATE  )
    target_compile_options(test_api PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(test_api PRIVATE  )
    target_compile_options(test_fileload PRIVATE -O3 -O2 -Wall -Wextra)
    target_link_options(test_fileload PRIVATE  )
endif()
