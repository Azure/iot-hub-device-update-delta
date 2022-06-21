add_executable(dumpdiff dumpdiff.cpp)

target_link_libraries(dumpdiff PRIVATE adudiffapi)

target_include_directories(dumpdiff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/diffs/api)