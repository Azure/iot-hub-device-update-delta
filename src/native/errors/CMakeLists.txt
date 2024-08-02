add_library (errors STATIC adu_log.cpp user_exception.cpp)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(errors PUBLIC fmt::fmt-header-only)

target_include_directories (errors PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})