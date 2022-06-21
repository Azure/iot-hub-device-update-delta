add_executable(io_utility_gtest io_utility_gtest.cpp)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(io_utility_gtest PRIVATE GTest::gmock GTest::gtest GTest::gmock_main GTest::gtest_main)
target_link_libraries(io_utility_gtest PRIVATE hash_utility error_utility io_utility)

add_test(NAME io_utility_test COMMAND io_utility_gtest --test_data_root ${CMAKE_SOURCE_DIR}/../data)

target_include_directories (io_utility_gtest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})