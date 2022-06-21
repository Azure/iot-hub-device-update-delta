add_executable (diffs_gtest diffs_gtest.cpp)

find_package(ZLIB REQUIRED)
find_package(zstd CONFIG REQUIRED)

target_link_libraries(diffs_gtest PUBLIC diffs)

target_include_directories(diffs_gtest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/diffs
    ${PROJECT_SOURCE_DIR}/error_utility
    ${PROJECT_SOURCE_DIR}/hash_utility
    ${PROJECT_SOURCE_DIR}/io_utility
    ${ZLIB_INCLUDE_DIRS}
    )

find_package(GTest CONFIG REQUIRED)
target_link_libraries(diffs_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)

