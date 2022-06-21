cmake_minimum_required (VERSION 3.1.0)
project(adu_diffs VERSION 1.0.3)

if (VCPKG_TARGET_TRIPLET STREQUAL "x64-mingw-static")
    message("Setting MINGW")
    set(MINGW 1)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(MAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
set(MAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-whole-archive")

if (WIN32 OR MINGW)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
else()
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D_GLIBCXX_PARALLEL")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_PARALLEL -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} -D_GLIBCXX_PARALLEL -g")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(error_utility)
add_subdirectory(hash_utility)

if (MINGW)
    message(WARNING "Detected MINGW - only building dumpextfs dependencies")
    set(DUMPEXTFS_DEPENDENCIES_ONLY ON)
endif()

if (NOT DUMPEXTFS_DEPENDENCIES_ONLY)
    add_subdirectory(io_utility)
    add_subdirectory(io_utility/gtest)
    add_subdirectory(diffs)
    add_subdirectory(diffs/gtest)
    add_subdirectory(diffs/api)
    add_subdirectory(tools/applydiff)
    add_subdirectory(tools/dumpdiff)
    add_subdirectory(tools/zstd_compress_file)

    if (UNIX)
        add_subdirectory(tools/dumpextfs)
        add_subdirectory(packaging/debian)
    endif()

    install(TARGETS adudiffapi)
endif()

