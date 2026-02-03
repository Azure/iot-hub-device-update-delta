add_executable(dumpextfs
    dump_json.cpp
    load_ext4.cpp
    main.cpp
    )

find_package(e2fsprogs REQUIRED)
target_include_directories(dumpextfs PRIVATE ${E2FSPROGS_INCLUDE_DIRS})
target_link_libraries(dumpextfs PRIVATE ${E2FSPROGS_LIBRARIES})

target_link_libraries(dumpextfs PRIVATE hashing)

find_package(jsoncpp CONFIG REQUIRED)
target_link_libraries(dumpextfs PRIVATE JsonCpp::JsonCpp)

if (WIN32)
    target_link_libraries(dumpextfs PRIVATE Ws2_32)
else()
target_link_libraries(dumpextfs PRIVATE -static)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_link_libraries(dumpextfs PRIVATE Threads::Threads)
endif()

target_include_directories(dumpextfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(dumpextfs
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)