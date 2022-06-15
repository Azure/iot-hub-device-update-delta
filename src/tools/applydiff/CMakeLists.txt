add_executable (applydiff applydiff.c)

target_link_libraries(applydiff PRIVATE adudiffapi)

target_include_directories (applydiff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/diffs/api)