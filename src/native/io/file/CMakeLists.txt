add_library(io_file STATIC 
    io_device.cpp
    binary_file_writer.cpp
    file.cpp
	)
    
if(UNIX)
    target_compile_options(io_file PRIVATE -fPIC)
endif()

target_link_libraries(io_file PUBLIC errors io)

target_include_directories(io_file PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_SOURCE_DIR}
	)
