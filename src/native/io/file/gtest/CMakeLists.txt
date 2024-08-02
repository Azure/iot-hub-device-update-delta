add_executable(io_file_gtest 
	main.cpp
	test_binary_file_writer.cpp
	test_io_device.cpp
)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_file_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_file_gtest PRIVATE 
	hashing
	errors 
	io
	test_utility 
	io_file
	)

if(UNIX)
	target_link_libraries(io_file_gtest PRIVATE stdc++fs)
endif()

add_test(NAME io_file_gtest COMMAND io_file_gtest --test_data_root ${CMAKE_SOURCE_DIR}/../data)

target_include_directories (io_file_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(io_file_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)