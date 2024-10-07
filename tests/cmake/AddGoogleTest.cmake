#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Build gtest as a static lib
set(BUILD_SHARED_LIBS OFF)

include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY      https://github.com/google/googletest.git
    GIT_TAG             v1.15.2
    EXCLUDE_FROM_ALL

)

FetchContent_MakeAvailable(googletest)

mark_as_advanced(
    gtest_force_shared_crt
    gmock_build_tests
    gtest_build_samples
    gtest_build_tests
    gtest_disable_pthreads
    gtest_hide_internal_symbols
    BUILD_GMOCK
    BUILD_GTEST
)
