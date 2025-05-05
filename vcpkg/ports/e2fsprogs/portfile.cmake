set(VERSION 1.47.2)

message(WARNING "${PORT} currently requires the following packages:\n"
 "  x64: gcc-x86-64-linux-gnu g++-x86-64-linux-gnu binutils-x86-64-linux-gnu pkg-config-x86-64-linux-gnu\n"
 "  arm: gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf pkg-config-arm-linux-gnueabihf\n"
 "  arm64: gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu pkg-config-aarch64-linux-gnu\n"
 )

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO tytso/e2fsprogs
    REF ff64357f839071968a727f8b5a08b0ddaedc5bbb
    SHA512 af19134b97df86ce33564ef0a7f357125af4171d8a1862f775e6404ca6e104223b06b2eeaea6064ba184c410688ca8f95901389b5562bbb551767e230615cdde
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
    REF ff64357f839071968a727f8b5a08b0ddaedc5bbb
    SHA512 af19134b97df86ce33564ef0a7f357125af4171d8a1862f775e6404ca6e104223b06b2eeaea6064ba184c410688ca8f95901389b5562bbb551767e230615cdde
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
