add_library(io_sequential STATIC
	reader.cpp
	reader_impl.cpp
	writer.cpp
	writer_impl.cpp
	)

if(UNIX)
    target_compile_options(io_sequential PRIVATE -fPIC)
endif()

target_link_libraries(io_sequential PUBLIC io errors)

target_include_directories(io_sequential PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	io
)