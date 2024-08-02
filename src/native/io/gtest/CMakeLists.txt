add_executable(io_gtest 
	main.cpp
	nul_io_device_test.cpp
	io_device_test.cpp
	io_device_view_test.cpp
	reader_test.cpp)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_gtest PRIVATE errors io)

add_test(NAME io_gtest COMMAND io_gtest)

target_include_directories(io_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(io_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)