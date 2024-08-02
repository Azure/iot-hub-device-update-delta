add_executable(makecpio
	makecpio_tool.cpp
	)

target_link_libraries(makecpio
	PRIVATE
	io_sequential
	io_file
	io
	cpio_archives
	)

target_include_directories(makecpio PUBLIC ${CMAKE_SOURCE_DIR})

set_target_properties(makecpio
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)