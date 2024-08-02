add_library(test_utility STATIC 
	buffer_helpers.cpp
	reader_validation.cpp
	random_data_file.cpp
	)

target_link_libraries(test_utility PUBLIC errors hashing io diffs_core)

target_include_directories(test_utility PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}
	)