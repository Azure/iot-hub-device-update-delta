add_executable(io_compressed_gtest
	compression_util.cpp
	common.cpp
	main.cpp
	test_bspatch_decompression_reader.cpp
	test_zlib_compression_reader.cpp
	test_zlib_compression_writer.cpp
	test_zlib_decompression_reader.cpp
	test_zstd_compression_reader.cpp
	test_zstd_compression_writer.cpp
	test_zstd_decompression_reader.cpp
	test_zstd_decompression_writer.cpp
	)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_compressed_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_compressed_gtest PRIVATE
	errors
	hashing
	test_utility
	io
	io_file
	io_sequential
	io_compressed
	)

if(UNIX)
	target_link_libraries(io_compressed_gtest PRIVATE stdc++fs)
endif()

add_test(NAME io_compressed_gtest COMMAND io_compressed_gtest)

target_include_directories(io_compressed_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(io_compressed_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)