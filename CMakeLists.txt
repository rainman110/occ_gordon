#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)

cmake_minimum_required (VERSION 3.15.0)
project (occ_gordon VERSION 1.0.0)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin")
endif(NOT DEFINED CMAKE_INSTALL_BINDIR)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

include(UseOpenCASCADE)

add_subdirectory(src)

#create gtests, override gtest standard setting
option(OCC_GORDON_BUILD_TESTS "Build occ_gordon Testsuite" OFF)

if(OCC_GORDON_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif(OCC_GORDON_BUILD_TESTS)
