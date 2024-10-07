#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#

# search for OpenCASCADE
set(CMAKE_PREFIX_PATH "$ENV{CASROOT};${CASROOT};${CMAKE_PREFIX_PATH}")

find_package(OpenCASCADE CONFIG REQUIRED)
option(OpenCASCADE_STATIC_LIBS "Should be checked, if static OpenCASCADE libs are linked" OFF)

message(STATUS "Found opencascade " ${OpenCASCADE_VERSION})

if (OpenCASCADE_WITH_TBB AND NOT OpenCASCADE_BUILD_SHARED_LIBS)
  set(TBB_FIND_QUIETLY 1)
  find_package(TBB REQUIRED)
  set_property(TARGET TKernel APPEND PROPERTY IMPORTED_LINK_INTERFACE_LIBRARIES TBB::tbb TBB::tbbmalloc)
endif()

set_property(TARGET TKernel APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS "Standard_EXPORT=")
set_target_properties(TKernel PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenCASCADE_INCLUDE_DIR}")

if (OCE_STATIC_LIBS OR OpenCASCADE_STATIC_LIBS)
    target_compile_definitions(TKernel INTERFACE HAVE_NO_DLL)
endif()
