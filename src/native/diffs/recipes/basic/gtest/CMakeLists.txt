add_executable (diffs_recipes_basic_gtest 
	common.cpp
	main.cpp
	test_all_zeros_recipe.cpp
	test_chain_recipe.cpp
	test_kitchen_slicing.cpp
	test_slice_recipe.cpp
	)

find_package(ZLIB REQUIRED)

# Use PkgConfig to find zstd since it doesn't provide CMake config in Yocto
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)

target_link_libraries(diffs_recipes_basic_gtest PUBLIC 
	diffs_recipes_basic
	test_utility
	)

target_include_directories(diffs_recipes_basic_gtest PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${ZLIB_INCLUDE_DIRS}
	)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(diffs_recipes_basic_gtest PUBLIC GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

set_target_properties(diffs_recipes_basic_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)