#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
#

add_executable(occ_gordon-apitest
    src/testSurfaceModeling.cpp
    src/main.cpp
)
target_link_libraries(occ_gordon-apitest PUBLIC gtest occ_gordon)
