add_library(io_hashed STATIC
	hashed_sequential_writer.cpp
	)

target_link_libraries(io_hashed PUBLIC hashing io errors)

if(UNIX)
	target_compile_options(io_hashed PRIVATE -fPIC)
endif()

target_include_directories(io_hashed PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	)
