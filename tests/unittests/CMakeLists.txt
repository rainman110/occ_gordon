#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#

file (GLOB test_SRCS *.cpp *.cxx *.cc *.C *.c *.h *.hpp)


add_executable(occ_gordon-unittests ${test_SRCS} )
target_link_libraries(occ_gordon-unittests PUBLIC gtest occ_gordon_internal occ_gordon geomlCommonTestUtils)

add_custom_command(
        TARGET occ_gordon-unittests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/TestData
                ${CMAKE_CURRENT_BINARY_DIR}/TestData)

#create directory to avoid crash in unit tests
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TestData/export)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TestData/analysis)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/TestData/bugs/505)
