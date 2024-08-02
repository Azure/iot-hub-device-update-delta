add_executable (diffs_core_gtest 
	common.cpp
    main.cpp
	test_cookbook.cpp
	test_item_definition.cpp
	test_kitchen.cpp
	test_pantry.cpp
	test_prepared_item.cpp		
    )

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

target_link_libraries(diffs_core_gtest PUBLIC 
	diffs_core
	test_utility
	)

target_include_directories(diffs_core_gtest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    )

find_package(GTest CONFIG REQUIRED)
target_link_libraries(diffs_core_gtest PUBLIC GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

set_target_properties(diffs_core_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)