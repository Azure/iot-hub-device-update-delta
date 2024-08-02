add_library(io STATIC
	reader.cpp
	all_zeros_io_device.cpp
	)

if(UNIX)
    target_compile_options(io PRIVATE -fPIC)
endif()

target_link_libraries(io PUBLIC errors hashing)

target_include_directories(io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})