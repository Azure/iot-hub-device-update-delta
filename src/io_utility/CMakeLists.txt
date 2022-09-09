add_library(io_utility STATIC 
    binary_file_reader.cpp
    binary_file_readerwriter.cpp
    binary_file_writer.cpp
    chained_reader.cpp
    child_hashed_writer.cpp 
    child_reader.cpp
    file.cpp
    sequential_reader.cpp
    sequential_writer.cpp
    stored_blob_reader.cpp
    stored_blob_reader_factory.cpp
    temp_file_backed_reader.cpp
    wrapped_writer_sequential_hashed_writer.cpp
    zlib_decompression_reader.cpp
    zstd_compression_writer.cpp 
    zstd_compression_reader.cpp
    zstd_decompression_writer.cpp
    zstd_decompression_reader.cpp
    bspatch_reader.cpp
)
    
if(UNIX)
    target_compile_options(io_utility PRIVATE -fPIC)
endif()
 
find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(io_utility PRIVATE Microsoft.GSL::GSL)

target_link_libraries(io_utility PUBLIC error_utility)

if(UNIX)
target_link_libraries(io_utility PUBLIC zstd::libzstd_static)
target_link_libraries(io_utility PRIVATE ZLIB::ZLIB)
target_link_libraries(io_utility PUBLIC ${ZLIB_LIBRARIES})
elseif(WIN32)
target_link_libraries(io_utility PUBLIC zstd::libzstd_shared)
target_link_libraries(io_utility PRIVATE ZLIB::ZLIB)
endif()

find_package(bsdiff REQUIRED)
target_link_libraries(io_utility PUBLIC ${BSDIFF_LIBRARIES})

target_include_directories(io_utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
	${PROJECT_SOURCE_DIR}/error_utility
	${PROJECT_SOURCE_DIR}/hash_utility
	${ZLIB_INCLUDE_DIRS}
	${MS_GSL_INCLUDE_DIR}
	${BSDIFF_INCLUDE_DIRS}
)