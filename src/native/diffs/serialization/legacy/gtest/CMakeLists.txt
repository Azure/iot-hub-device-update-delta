add_executable (diffs_serialization_legacy_gtest
    main.cpp
	test_serializer.cpp
    )

target_link_libraries(diffs_serialization_legacy_gtest PUBLIC 
    test_utility
	diffs_serialization_legacy
    io_file
	)

target_include_directories(diffs_serialization_legacy_gtest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

if (UNIX)
	target_link_libraries(diffs_serialization_legacy_gtest PRIVATE stdc++fs)
endif()

find_package(GTest CONFIG REQUIRED)
target_link_libraries(diffs_serialization_legacy_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

set_target_properties(diffs_serialization_legacy_gtest
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/bin"
	)