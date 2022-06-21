add_library(adudiffapi SHARED
    adudiffapply.cpp
    adudiffcreate.cpp
    apply_session.cpp
    create_session.cpp
    )

add_definitions(-DADUDIFFAPI_DLL_EXPORTS)

target_link_libraries(adudiffapi PUBLIC diffs)

target_include_directories(adudiffapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
