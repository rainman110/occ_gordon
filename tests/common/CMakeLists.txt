#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#

# library common for test utils

add_library(geomlCommonTestUtils INTERFACE)

target_include_directories(geomlCommonTestUtils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
