set(ADUDIFFAPI_SOURCE_FILES 
    adudiffapply.cpp
    adudiffcreate.cpp
    apply_session.cpp
    create_session.cpp
    )

if (WIN32 OR MINGW)
    list(APPEND ADUDIFFAPI_SOURCE_FILES adudiffapi.rc)
endif ()

add_library(adudiffapi SHARED
    ${ADUDIFFAPI_SOURCE_FILES}
    )

SET(VER_ADUDIFFAPI_VERSION "1,0,0" CACHE STRING "Version for adudiffapi.dll")
SET(VER_ADUDIFFAPI_VERSION_STRING "0.0.0.YYMMDDDD" CACHE STRING "Version(string) for adudiffapi.dll")

add_definitions(-DADUDIFFAPI_DLL_EXPORTS -DVER_ADUDIFFAPI_VERSION=${VER_ADUDIFFAPI_VERSION} -DVER_ADUDIFFAPI_VERSION_STRING=${VER_ADUDIFFAPI_VERSION_STRING})

target_link_libraries(adudiffapi PUBLIC diffs)

target_include_directories(adudiffapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

