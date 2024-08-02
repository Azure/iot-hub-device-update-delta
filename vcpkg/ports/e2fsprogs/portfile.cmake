set(VERSION 1.47)

message(WARNING "${PORT} currently requires the following packages:\n"
 "  x64: gcc-x86-64-linux-gnu g++-x86-64-linux-gnu binutils-x86-64-linux-gnu pkg-config-x86-64-linux-gnu\n"
 "  arm: gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf pkg-config-arm-linux-gnueabihf\n"
 "  arm64: gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu pkg-config-aarch64-linux-gnu\n"
 )

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO tytso/e2fsprogs
    REF 950a0d69c82b585aba30118f01bf80151deffe8c
    SHA512 b0fbbdd6d55c1de9cd9bc1a8211112ec5ce41fd5bc886070cb7a1d0cb504615d322b6d488b68f7cf99bca43e7e6ef17bc66dd36e3ac75b7e692395535fbca6ca
    HEAD_REF main
    PATCHES
        get_current_physblock.patch
		)

if (VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
	set(HOST "x86_64-linux-gnu")
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
	set(HOST "aarch64-linux-gnu")
elseif (VCPKG_TARGET_ARCHITECTURE STREQUAL "arm")
	set(HOST "arm-linux-gnueabihf")
endif()

vcpkg_configure_make(
	SOURCE_PATH ${SOURCE_PATH}
	OPTIONS
		--target=${HOST}
		--host=${HOST}
		)

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

else()
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO tytso/e2fsprogs
    REF 950a0d69c82b585aba30118f01bf80151deffe8c
    SHA512 b0fbbdd6d55c1de9cd9bc1a8211112ec5ce41fd5bc886070cb7a1d0cb504615d322b6d488b68f7cf99bca43e7e6ef17bc66dd36e3ac75b7e692395535fbca6ca
    HEAD_REF main
    PATCHES
        get_current_physblock.patch
		windows-msvc.patch
		)

vcpkg_configure_cmake(
	SOURCE_PATH ${SOURCE_PATH}
	PREFER_NINJA
	)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(INSTALL
    "${CMAKE_CURRENT_LIST_DIR}/e2fsprogs-config.cmake"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
	)

file(INSTALL
	${SOURCE_PATH}/lib/ext2fs/bitops.h
    ${SOURCE_PATH}/lib/ext2fs/ext2_err.h
	${SOURCE_PATH}/lib/ext2fs/ext2_ext_attr.h
	${SOURCE_PATH}/lib/ext2fs/ext2_fs.h
	${SOURCE_PATH}/lib/ext2fs/ext2_io.h
	${SOURCE_PATH}/lib/ext2fs/ext2_types.h
	${SOURCE_PATH}/lib/ext2fs/ext2fs.h
	${SOURCE_PATH}/lib/ext2fs/ext3_extents.h
	${SOURCE_PATH}/lib/ext2fs/hashmap.h
	${SOURCE_PATH}/lib/ext2fs/qcow2.h
	${SOURCE_PATH}/lib/ext2fs/tdb.h
    DESTINATION ${CURRENT_PACKAGES_DIR}/include/ext2fs
	)

file(INSTALL
	${SOURCE_PATH}/lib/et/com_err.h
    DESTINATION ${CURRENT_PACKAGES_DIR}/include
	)

file(INSTALL
	${SOURCE_PATH}/lib/et/com_err.h
    DESTINATION ${CURRENT_PACKAGES_DIR}/include/et
	)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

find_path(EXT2_FS_INCLUDE_DIR ext2_fs.h PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include" NO_DEFAULT_PATH)
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/e2fsprogs)

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/share)
file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/share/e2fsprogs/info)
file(INSTALL ${SOURCE_PATH}/NOTICE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
endif()
