set(VERSION 1.46.5)

message(WARNING "${PORT} currently requires the following packages:\n"
 "  x64: gcc-x86-64-linux-gnu g++-x86-64-linux-gnu binutils-x86-64-linux-gnu pkg-config-x86-64-linux-gnu\n"
 "  arm: gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf pkg-config-arm-linux-gnueabihf\n"
 "  arm64: gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu pkg-config-aarch64-linux-gnu\n"
 "  x64-mingw: gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64 binutils-mingw-w64-x86-64 pkg-config-mingw-w64-x86-64")

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO tytso/e2fsprogs
    REF 96185e9bef1dca5a3b7d93f244d65107aad5f37f
    SHA512 3cc5e947f16cf66eca109be3594362feb3b5f64bfbc51fc6e9eda05d525fc1d7b9bdd76a2d72be28a942446324f19020b6126c262ce6ddb0b874a5ddbd490a1e
    HEAD_REF dev
    PATCHES
        get_current_physblock.patch
        windows-build-necessary.patch
)

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Building outside of linux is not supported.")
endif()

if(UNIX)
    if(${VCPKG_TARGET_IS_MINGW})
        set(HOST "x86_64-w64-mingw32")
        set(VCPKG_CXX_FLAGS "-DWIN32 -DHAVE_WINSOCK_H -I${SOURCE_PATH}/include/mingw ${VCPKG_CXX_FLAGS}")
        set(VCPKG_C_FLAGS "-DWIN32 -DHAVE_WINSOCK_H -I${SOURCE_PATH}/include/mingw ${VCPKG_C_FLAGS}")
    elseif (${VCPKG_CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
            set(HOST "x86_64-linux-gnu")
        elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
            set(HOST "aarch64-linux-gnu")
        elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm")
            set(HOST "arm-linux-gnueabihf")
        endif()
    endif()
endif()

if(HOST STREQUAL "")
    message(FATAL_ERROR "Unsupported build. Only linux build environments are supported. Valid targets are mingw-w64, x64, arm64 and arm.")
endif()

if(${VCPKG_TARGET_IS_MINGW})
    vcpkg_configure_make(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
            --target=${HOST}
            --host=${HOST}
            --prefix=/usr/x86_64-w64-mingw32
            --disable-defrag
            --disable-resizer
            --disable-debugfs
            --disable-imager
            --disable-mmp
            --disable-tdb
            --disable-bmap-stats
            --disable-nls
        )
else()
    vcpkg_configure_make(
        SOURCE_PATH ${SOURCE_PATH}
        OPTIONS
            --target=${HOST}
            --host=${HOST}
            )
endif()

vcpkg_install_make()
vcpkg_copy_pdbs()

file(INSTALL
    "${CMAKE_CURRENT_LIST_DIR}/e2fsprogs-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)

find_path(EXT2_FS_INCLUDE_DIR ext2_fs.h PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include" NO_DEFAULT_PATH)
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/e2fsprogs)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/share/e2fsprogs/info)
file(INSTALL ${SOURCE_PATH}/NOTICE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
