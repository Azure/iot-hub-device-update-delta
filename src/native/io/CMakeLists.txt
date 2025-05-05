add_library(io STATIC
	all_zeros_io_device.cpp
	reader.cpp
	writer.cpp
	uint64_t_endian.cpp
	)


if(UNIX)
    target_compile_options(io PRIVATE -fPIC)
endif()

if(WIN32)
	target_link_libraries(io PUBLIC wsock32 ws2_32)
endif()

target_link_libraries(io PUBLIC errors hashing)

target_include_directories(io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})