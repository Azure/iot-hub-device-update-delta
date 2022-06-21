add_executable(dumpextfs
    dump_json.cpp
    file_details.cpp
    load_ext4.cpp
    main.cpp
    )

find_package(e2fsprogs REQUIRED)

target_link_libraries(dumpextfs PRIVATE -static)

target_link_libraries(dumpextfs PRIVATE hash_utility)
target_link_libraries(dumpextfs PRIVATE ${E2FSPROGS_LIBRARIES})

if (NOT MINGW)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_link_libraries(dumpextfs PRIVATE Threads::Threads)
endif()

target_include_directories(dumpextfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${E2FSPROGS_INCLUDE_DIRS})