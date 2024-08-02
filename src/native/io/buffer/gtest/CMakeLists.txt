add_executable(io_buffer_gtest 
	common.cpp
	main.cpp
	test_reader_and_writer_interaction.cpp
	test_io_device.cpp
	test_reader_factory.cpp
	test_writer.cpp
	)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_buffer_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_buffer_gtest PRIVATE errors io)

add_test(NAME io_buffer_gtest COMMAND io_buffer_gtest)

target_include_directories (io_buffer_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(io_buffer_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)