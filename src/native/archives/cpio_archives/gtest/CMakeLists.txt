add_executable(cpio_archives_gtest
	main.cpp
	test_create_archive.cpp
	)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(cpio_archives_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(cpio_archives_gtest PRIVATE errors io io_file cpio_archives)

add_test(NAME cpio_archives_gtest COMMAND cpio_archives_gtest)

target_include_directories (cpio_archives_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(cpio_archives_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)