add_executable(occ_gordon-single-header-test
    src/main.cpp
)
add_dependencies(occ_gordon-single-header-test occ_gordon-single-header)

target_compile_features(occ_gordon-single-header-test PRIVATE cxx_std_17)
target_include_directories(occ_gordon-single-header-test PRIVATE "${OCC_GORDON_SINGLE_HEADER_DIR}")
target_link_libraries(occ_gordon-single-header-test PUBLIC ${OCC_GORDON_OCCT_LIBRARIES})

add_test(NAME occ_gordon-single-header-test COMMAND occ_gordon-single-header-test)
