# Unit tests for NTREG — uses header-only criterion_shim.hpp
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include/compat)

set(CLI_EXES
    cm_core_tests
    cm_value_tests
    cm_value_adv_tests
    cm_index_tests
    cm_check_tests
    cm_hive_tests
    cm_path_tests
    cm_api_tests
)

foreach(name ${CLI_EXES})
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE reg)
  if(MSVC)
    target_compile_options(${name} PRIVATE /O2 /W4)
  else()
    target_compile_options(${name} PRIVATE -O2 -g -Wall -Wextra -Wpedantic)
  endif()
endforeach()

add_custom_target(all_cli_tests DEPENDS ${CLI_EXES})
add_custom_target(unit_tests_all DEPENDS reg_unit_tests ${CLI_EXES})

if(CMAKE_PROJECT_NAME STREQUAL ${PROJECT_NAME})
  enable_testing()
  foreach(name ${CLI_EXES})
    add_test(NAME unit.${name} COMMAND ${name})
  endforeach()
endif()
