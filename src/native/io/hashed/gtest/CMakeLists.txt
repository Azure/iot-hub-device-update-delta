add_executable(io_hashed_gtest io_hashed_gtest.cpp)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_hashed_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_hashed_gtest PRIVATE 
	hashing
	errors
	io
	io_file
	io_sequential
	io_hashed
	)

if (UNIX)
	target_link_libraries(io_hashed_gtest PRIVATE stdc++fs)
endif()

add_test(NAME io_test COMMAND io_hashed_gtest)

target_include_directories (io_hashed_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(io_hashed_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)