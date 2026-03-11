#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#
# To build this project
#  - export CMAKE_PREFIX_PATH=path/to/pythonocc-code
#  - pip install .

cmake_minimum_required(VERSION 3.4)
project(occ_gordon-python VERSION 0.1)

set(CMAKE_INSTALL_RPATH $ORIGIN)

if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD)
endif (POLICY CMP0086)

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD)
endif (POLICY CMP0078)
if (POLICY CMP0169)
  # We intentionally use FetchContent_Populate for source-only downloads and do
  # not want FetchContent_MakeAvailable semantics (which may add subdirectories).
  cmake_policy(SET CMP0169 OLD)
endif (POLICY CMP0169)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(OccGordonPythonBindings)

# build geoml first
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_BINARY_DIR}/native EXCLUDE_FROM_ALL)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 interpreter:" ${Python3_EXECUTABLE})
message(STATUS "Python include directory: ${Python3_INCLUDE_DIR}")
message(STATUS "Python library release: ${Python3_LIBRARY_RELEASE}")

option(OCC_GORDON_FETCH_PYTHONOCC_SOURCE
       "Automatically fetch pythonocc-core sources if PythonOCC_SOURCE_DIR is not provided."
       ON)
set(OCC_GORDON_PYTHONOCC_REPOSITORY "https://github.com/tpaviot/pythonocc-core.git"
    CACHE STRING "Git repository URL for pythonocc-core sources.")
set(OCC_GORDON_PYTHONOCC_GIT_TAG ""
    CACHE STRING "Override git tag for pythonocc-core sources. If empty, detected pythonocc version is used.")

occ_gordon_detect_installed_pythonocc_version("${Python3_EXECUTABLE}" PYTHONOCC_INSTALLED_VERSION)
message(STATUS "Detected installed pythonocc version: ${PYTHONOCC_INSTALLED_VERSION}")

occ_gordon_resolve_pythonocc_source_dir(PYTHONOCC_INSTALLED_VERSION ${PYTHONOCC_INSTALLED_VERSION})

find_package(SWIG 3.0.11 REQUIRED)
find_package(PythonOCC REQUIRED)

option(OCC_GORDON_SKIP_SWIG_VERSION_CHECK
       "Skip compatibility check between pythonocc-generated wrappers and SWIG executable."
       OFF)
option(OCC_GORDON_ENFORCE_EXACT_SWIG_VERSION
       "Require exact SWIG version match in addition to SWIG runtime ABI compatibility."
       OFF)

if (NOT OCC_GORDON_SKIP_SWIG_VERSION_CHECK)
  occ_gordon_validate_swig_compatibility("${Python3_EXECUTABLE}")
endif()


include(${SWIG_USE_FILE})

# we use the same definitions as pythonocc for their build to hide some of their warnings
option(SWIG_HIDE_WARNINGS "Check this option if you want a less verbose swig output." ON)
if(SWIG_HIDE_WARNINGS)
  message(STATUS "Disabled SWIG warnings")
  set(CMAKE_SWIG_FLAGS ${CMAKE_SWIG_FLAGS} -w302,401,402,412,314,509,512,504,325,503,520,350,351,383,389,394,395,404)
endif()


include_directories(${PYTHON_INCLUDE_PATH})

include_directories(${PythonOCC_INCLUDE_DIRS})

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories($<TARGET_PROPERTY:occ_gordon,INTERFACE_INCLUDE_DIRECTORIES>)

set_source_files_properties(occ_gordon_native.i PROPERTIES CPLUSPLUS ON)
set(SWIG_MODULE_occ_gordon_native_EXTRA_DEPS common.i)

swig_add_library(occ_gordon_native LANGUAGE python SOURCES occ_gordon_native.i TYPE MODULE)
swig_link_libraries(occ_gordon_native occ_gordon Python3::Module)

install(TARGETS _occ_gordon_native LIBRARY DESTINATION occ_gordon)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/occ_gordon_native.py
        DESTINATION occ_gordon
)


