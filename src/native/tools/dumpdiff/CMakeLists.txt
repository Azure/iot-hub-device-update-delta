add_executable(dumpdiff dumpdiff.cpp)

target_link_libraries(dumpdiff PRIVATE diffs_serialization_legacy diffs_serialization_standard io_file)

if (UNIX)
	target_link_libraries(dumpdiff PRIVATE stdc++fs)
endif()

target_include_directories(dumpdiff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(dumpdiff
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)