add_library (error_utility STATIC user_exception.cpp)

target_include_directories (error_utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})