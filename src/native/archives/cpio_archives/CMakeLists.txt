add_library(cpio_archives STATIC
	cpio_archive.cpp
	cpio_file.cpp
	hexadecimal_data.cpp
	octal_data.cpp
	string_data.cpp
	)

target_link_libraries(cpio_archives PUBLIC errors io io_sequential)

target_include_directories(cpio_archives PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	)