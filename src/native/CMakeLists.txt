cmake_minimum_required (VERSION 3.15.0)

set(ADU_DIFFS_VERSION, "2.0.0")
project(adu_diffs VERSION ${ADU_DIFFS_VERSION})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (WIN32)
string(REGEX REPLACE "/w[1|2|3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[1|2|3|4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
add_compile_options("/Zi;/sdl;/guard:cf;/DEBUG;/W4;/Qspectre")

set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:ICF")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /OPT:ICF")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:ICF")

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

else()

add_compile_options("-g;-dI;-Wno-psabi;-Wall")

endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(errors)
add_subdirectory(hashing)
add_subdirectory(io)
add_subdirectory(io/gtest)
add_subdirectory(io/buffer/gtest)
add_subdirectory(io/compressed)
add_subdirectory(io/compressed/gtest)
add_subdirectory(io/file)
add_subdirectory(io/file/gtest)
add_subdirectory(io/hashed)
add_subdirectory(io/hashed/gtest)
add_subdirectory(io/sequential)
add_subdirectory(diffs/api)
#	add_subdirectory(diffs/api/gtest)
add_subdirectory(diffs/core)
add_subdirectory(diffs/core/gtest)
add_subdirectory(diffs/recipes/basic)
add_subdirectory(diffs/recipes/basic/gtest)
add_subdirectory(diffs/recipes/compressed)
add_subdirectory(diffs/recipes/compressed/gtest)
add_subdirectory(diffs/serialization/legacy)
add_subdirectory(diffs/serialization/legacy/gtest)
add_subdirectory(diffs/serialization/standard)
#	add_subdirectory(diffs/serialization/standard/gtest)
add_subdirectory(archives/cpio_archives)
add_subdirectory(archives/cpio_archives/gtest)
add_subdirectory(test_utility)
add_subdirectory(tools/applydiff)
add_subdirectory(tools/dumpdiff)
add_subdirectory(tools/dumpextfs)
add_subdirectory(tools/makecpio)
add_subdirectory(tools/recompress)
add_subdirectory(tools/zstd_compress_file)

if (UNIX)
    add_subdirectory(packaging/debian)
endif()

install(TARGETS adudiffapi)


