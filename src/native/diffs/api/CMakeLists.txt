set(ADUDIFFAPI_SOURCE_FILES
	aduapi_type_conversion.cpp
	apply_session.cpp
	create_session.cpp
	diff_api.cpp
	diffa_api.cpp
	diffc_api.cpp
	legacy_apply_session.cpp
	legacy_adudiffapply.cpp
	)

if (WIN32 OR MINGW)
	list(APPEND ADUDIFFAPI_SOURCE_FILES adudiffapi.rc)
endif ()

add_library(adudiffapi SHARED
	${ADUDIFFAPI_SOURCE_FILES}
	 "aduapi_types.h" "aduapi_type_conversion.cpp")

add_library(adudiffapi_static STATIC
	${ADUDIFFAPI_SOURCE_FILES}
	 "aduapi_types.h" "aduapi_type_conversion.cpp")

# Set the output name for the static library to match install expectations
set_target_properties(adudiffapi_static PROPERTIES OUTPUT_NAME adudiffapi)

SET(VER_ADUDIFFAPI_VERSION "1,0,0" CACHE STRING "Version for adudiffapi.dll")
SET(VER_ADUDIFFAPI_VERSION_STRING "0.0.0.YYMMDDDD" CACHE STRING "Version(string) for adudiffapi.dll")

add_definitions(-DADUDIFFAPI_DLL_EXPORTS -DVER_ADUDIFFAPI_VERSION=${VER_ADUDIFFAPI_VERSION} -DVER_ADUDIFFAPI_VERSION_STRING=${VER_ADUDIFFAPI_VERSION_STRING})

target_link_libraries(adudiffapi
	PUBLIC
	diffs_core
	diffs_serialization_legacy
	diffs_serialization_standard
	io_file
	)

target_link_libraries(adudiffapi_static
	PUBLIC
	diffs_core
	diffs_serialization_legacy
	diffs_serialization_standard
	io_file
	)

target_include_directories(adudiffapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_include_directories(adudiffapi_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set_target_properties(adudiffapi
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)

set_target_properties(adudiffapi_static
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	)