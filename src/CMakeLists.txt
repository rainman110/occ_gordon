#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2018 German Aerospace Center (DLR)
#
# occ_gordon Library

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(occ_gordon_internal OBJECT
    internal/ApproxResult.h
    internal/BSplineAlgorithms.cpp
    internal/BSplineAlgorithms.h
    internal/BSplineApproxInterp.cpp
    internal/BSplineApproxInterp.h
    internal/CurveNetworkSorter.cpp
    internal/CurveNetworkSorter.h
    internal/CurvesToSurface.cpp
    internal/CurvesToSurface.h
    internal/GordonSurfaceBuilder.cpp
    internal/GordonSurfaceBuilder.h
    internal/InterpolateCurveNetwork.cpp
    internal/InterpolateCurveNetwork.h
    internal/IntersectBSplines.cpp
    internal/IntersectBSplines.h
    internal/IntersectionPoint.h
    internal/PointsToBSplineInterpolation.cpp
    internal/PointsToBSplineInterpolation.h
    internal/occ_gordon_internal.h
    internal/occ_std_adapters.h
    internal/Error.cpp
)


target_link_libraries(occ_gordon_internal PUBLIC ${OpenCASCADE_LIBRARIES})
target_compile_features(occ_gordon_internal PRIVATE cxx_std_17)

target_include_directories(occ_gordon_internal
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
        ${CMAKE_CURRENT_BINARY_DIR}

        ${CMAKE_CURRENT_SOURCE_DIR}
)


# object library containing just the compiled sources
add_library(occ_gordon
    occ_gordon/occ_gordon.h
    occ_gordon/occ_gordon.cpp
    internal/Error.h

    $<TARGET_OBJECTS:occ_gordon_internal>
)
target_compile_features(occ_gordon PRIVATE cxx_std_17)

include(GenerateExportHeader)
generate_export_header(occ_gordon
    EXPORT_MACRO_NAME OCC_GORDON_EXPORT
    EXPORT_FILE_NAME occ_gordon/exports.h
)

target_link_libraries(occ_gordon PUBLIC ${OpenCASCADE_LIBRARIES})


if(MSVC)
    target_compile_definitions(occ_gordon PRIVATE _CRT_SECURE_NO_WARNINGS=1 /wd4355)
    target_link_libraries(occ_gordon PRIVATE ShlWapi PUBLIC Ws2_32)
endif(MSVC)

target_include_directories(occ_gordon PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set_target_properties( occ_gordon PROPERTIES VERSION ${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set_target_properties( occ_gordon PROPERTIES SOVERSION "${PROJECT_VERSION_MINOR}")

install (TARGETS occ_gordon
         EXPORT occ_gordon-targets
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY occ_gordon
    DESTINATION include
    FILES_MATCHING
    PATTERN "*.h"
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/occ_gordon/exports.h DESTINATION include/occ_gordon
)


# cmake configuration export
set(CMAKE_INSTALL_INCLUDE_DIR "include")
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/occ_gordon")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/occ_gordon-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)


file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/occ_gordon-config.cmake"
  INPUT  "${PROJECT_SOURCE_DIR}/cmake/occ_gordon-config.cmake.in"
)

install (EXPORT occ_gordon-targets DESTINATION ${CONFIG_INSTALL_DIR})

install (FILES
  "${CMAKE_CURRENT_BINARY_DIR}/occ_gordon-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/occ_gordon-config-version.cmake"
  DESTINATION ${CONFIG_INSTALL_DIR})
