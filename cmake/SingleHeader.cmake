#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 German Aerospace Center (DLR)
#

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(OCC_GORDON_SINGLE_HEADER_DIR "${PROJECT_BINARY_DIR}/single_header")
set(OCC_GORDON_SINGLE_HEADER_FILE "${OCC_GORDON_SINGLE_HEADER_DIR}/occ_gordon_single.hpp")

file(GLOB_RECURSE OCC_GORDON_SINGLE_HEADER_DEPENDS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.h"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/tools/amalgamation/*.in.hpp"
    "${PROJECT_SOURCE_DIR}/tools/amalgamate.py"
)

add_custom_command(
    OUTPUT "${OCC_GORDON_SINGLE_HEADER_FILE}"
    COMMAND "${Python3_EXECUTABLE}"
            "${PROJECT_SOURCE_DIR}/tools/amalgamate.py"
            --project-root "${PROJECT_SOURCE_DIR}"
            --template "${PROJECT_SOURCE_DIR}/tools/amalgamation/occ_gordon_single_header.in.hpp"
            --output "${OCC_GORDON_SINGLE_HEADER_FILE}"
    DEPENDS ${OCC_GORDON_SINGLE_HEADER_DEPENDS}
    VERBATIM
)

add_custom_target(occ_gordon-single-header
    DEPENDS "${OCC_GORDON_SINGLE_HEADER_FILE}"
)
