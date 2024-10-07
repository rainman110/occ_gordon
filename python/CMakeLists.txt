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

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# build geoml first
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ ${CMAKE_CURRENT_BINARY_DIR}/native EXCLUDE_FROM_ALL)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Python3 interpreter:" ${Python3_EXECUTABLE})
message(STATUS "Python include directory: ${Python3_INCLUDE_DIR}")
message(STATUS "Python library release: ${Python3_LIBRARY_RELEASE}")


# Define the function to get the OCC version from pythonOCC
function(get_OCC_version OCC_VERSION)
    # Execute the Python command to get the version
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import OCC; print('%d.%d.%d' % (OCC.PYTHONOCC_VERSION_MAJOR, OCC.PYTHONOCC_VERSION_MINOR, OCC.PYTHONOCC_VERSION_PATCH))"
        OUTPUT_VARIABLE OCC_VERSION_OUTPUT
        ERROR_VARIABLE OCC_ERROR_OUTPUT
        RESULT_VARIABLE OCC_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Check if the command was successful
    if (NOT OCC_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to get OpenCascade (pythonOCC) version: ${OCC_ERROR_OUTPUT}")
    endif()

    # Set the OCC version as a return value
    set(${OCC_VERSION} "${OCC_VERSION_OUTPUT}" PARENT_SCOPE)
endfunction()

get_OCC_version(pyocc_version)
message("PythonOCC Version" ${pyocc_version})

find_package(SWIG 3.0.11 REQUIRED)
find_package(PythonOCC REQUIRED)


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


