# FindCriterion.cmake  
# Minimal module to locate Criterion library for Cross-platform CMake builds

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CRITERION QUIET criterion >= 2.4.1)
endif()

find_path(CRITERION_INCLUDE_DIR NAMES criterion/criterion.h
  PATHS ${PC_CRITERION_INCLUDE_DIRS} 
  PATH_SUFFIXES include 
  HINTS $ENV{CRITERION_DIR} ENV CRITERION_DIR)

find_library(CRITERION_LIBRARY NAMES criterion libcriterion libcrit
  PATHS ${PC_CRITERION_LIBRARY_DIRS} 
  PATH_SUFFIXES lib lib64 
  HINTS $ENV{CRITERION_DIR} ENV CRITERION_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Criterion 
  REQUIRED_VARS CRITERION_LIBRARY CRITERION_INCLUDE_DIR 
  VERSION_VAR PC_CRITERION_VERSION)
)

if(CRITERION_FOUND)
  set(CRITERION_LIBRARIES ${CRITERION_LIBRARY})
  set(CRITERION_INCLUDE_DIRS ${CRITERION_INCLUDE_DIR})
  mark_as_advanced(CRITERION_INCLUDE_DIR CRITERION_LIBRARY CRITERION_LIBRARIES CRITERION_INCLUDE_DIRS)
  add_library(Criterion::criterion UNKNOWN IMPORTED)
  set_property(TARGET Criterion::criterion PROPERTY IMPORTED_LOCATION ${CRITERION_LIBRARIES})
  set_property(TARGET Criterion::criterion PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CRITERION_INCLUDE_DIRS})
endif()