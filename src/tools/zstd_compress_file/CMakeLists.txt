add_executable(zstd_compress_file zstd_compress_file.cpp compress_utility.cpp get_file_hash.cpp)

target_link_libraries(zstd_compress_file io_utility)
target_link_libraries(zstd_compress_file hash_utility)

if(UNIX)
target_link_libraries(zstd_compress_file "-static")
endif()