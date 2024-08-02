add_executable(recompress
	recompress.cpp
	recompress_tool.cpp
	swupdate_helpers.cpp
	)

find_package(libconfig CONFIG REQUIRED)

if(UNIX)
	target_link_libraries(recompress PRIVATE stdc++fs)
	target_link_libraries(recompress PRIVATE libconfig::config libconfig::config++)
else()
	target_link_libraries(recompress PRIVATE libconfig::libconfig libconfig::libconfig++)
endif()

target_link_libraries(recompress
	PRIVATE
	io_compressed
	io_sequential
	io_file
	io
	hashing
	cpio_archives
	)

target_include_directories(recompress PUBLIC ${CMAKE_SOURCE_DIR})

set_target_properties(recompress
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)