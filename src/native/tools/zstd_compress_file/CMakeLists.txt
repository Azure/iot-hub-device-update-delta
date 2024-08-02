add_executable(zstd_compress_file
	zstd_compress_file.cpp
	compress_utility.cpp
	get_file_hash.cpp
	)

target_link_libraries(zstd_compress_file
	PRIVATE
	io_compressed
	io_sequential
	io_file
	io
	hashing
	)

if(UNIX)
	target_link_libraries(zstd_compress_file PRIVATE stdc++fs)
endif()

target_include_directories(zstd_compress_file PUBLIC ${CMAKE_SOURCE_DIR})

set_target_properties(zstd_compress_file
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)