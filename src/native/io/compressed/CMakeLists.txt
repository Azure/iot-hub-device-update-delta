add_library(io_compressed STATIC
	bsdiff_compressor.cpp
	bspatch_decompression_reader.cpp
	writer_to_reader_channel.cpp
	zlib_compression_reader.cpp
	zlib_compression_writer.cpp
	zlib_decompression_reader.cpp
	zlib_decompression_writer.cpp
	zstd_compression_reader.cpp
	zstd_compression_writer.cpp
	zstd_decompression_reader.cpp
	zstd_decompression_writer.cpp
	bsdiff_stream_wrappers.cpp
	zstd_wrappers.cpp
	)

if(UNIX)
    target_compile_options(io_compressed PRIVATE -fPIC)
endif()

find_package(ZLIB REQUIRED)

# Use PkgConfig to find zstd since it doesn't provide CMake config in Yocto
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)

target_link_libraries(io_compressed PUBLIC errors io io_sequential)

if(UNIX)
target_link_libraries(io_compressed PUBLIC ${ZSTD_LIBRARIES})
target_include_directories(io_compressed PRIVATE ${ZSTD_INCLUDE_DIRS})
target_compile_options(io_compressed PRIVATE ${ZSTD_CFLAGS_OTHER})
target_link_libraries(io_compressed PRIVATE ZLIB::ZLIB)
target_link_libraries(io_compressed PUBLIC ${ZLIB_LIBRARIES})
elseif(WIN32)
target_link_libraries(io_compressed PUBLIC zstd::libzstd_shared)
target_link_libraries(io_compressed PRIVATE ZLIB::ZLIB)
endif()

# Use PkgConfig to find bsdiff since it doesn't provide CMake config in Yocto
pkg_check_modules(BSDIFF REQUIRED bsdiff)
target_include_directories(io_compressed PRIVATE ${BSDIFF_INCLUDE_DIRS})
target_link_libraries(io_compressed PRIVATE ${BSDIFF_LIBRARIES})

target_include_directories(io_compressed PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	${ZLIB_INCLUDE_DIRS}
	${BSDIFF_INCLUDE_DIRS}
	)
