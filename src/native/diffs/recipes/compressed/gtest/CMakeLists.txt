add_executable (diffs_recipes_compressed_gtest
    main.cpp
    test_bspatch_decompression_recipe.cpp
    test_zlib_compression_recipe.cpp
    test_zlib_decompression_recipe.cpp
    test_zstd_compression_recipe.cpp
    test_zstd_decompression_recipe.cpp
    )

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

target_link_libraries(diffs_recipes_compressed_gtest PUBLIC
	io_file
	diffs_recipes_basic
    diffs_recipes_compressed
    test_utility
    )

find_package(GTest CONFIG REQUIRED)
target_link_libraries(diffs_recipes_compressed_gtest PRIVATE
	GTest::gmock
	GTest::gtest
	GTest::gmock_main
	GTest::gtest_main
	)

if(UNIX)
	target_link_libraries(diffs_recipes_compressed_gtest PRIVATE stdc++fs)
endif()

target_include_directories(diffs_recipes_compressed_gtest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

set_target_properties(diffs_recipes_compressed_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)
