add_executable(recompress
	recompress.cpp
	recompress_tool.cpp
	swupdate_helpers.cpp
	)

# Use PkgConfig to find libconfig since it doesn't provide CMake config in Yocto
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBCONFIG REQUIRED libconfig++)
include_directories(${LIBCONFIG_INCLUDE_DIRS})
link_directories(${LIBCONFIG_LIBRARY_DIRS})

target_link_libraries(recompress PRIVATE stdc++fs)
target_link_libraries(recompress PRIVATE ${LIBCONFIG_LIBRARIES} config++)

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