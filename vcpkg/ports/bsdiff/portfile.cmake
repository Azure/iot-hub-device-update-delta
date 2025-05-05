vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO zhuyie/bsdiff
    REF 6b70bf123c725d181505c7bc181debf8236274e3
    SHA512 05df6c72e6100c49e17b1a416c7813cc3b42f12aaff4efc2031d631a08c66cc544f5659d488530a7e1358e95db878b2a2c2d1a07af2b1ba2dfe67fa04f22d609
    HEAD_REF master
    PATCHES
        bsdiff-cmakelists.patch
)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
       -DBUILD_STANDALONES=ON
       -DBUILD_SHARED_LIBS=ON
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

find_path(BSDIFF_INCLUDE_DIR bsdiff.h PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include" NO_DEFAULT_PATH)

# install target
file(INSTALL
    "${SOURCE_PATH}/include/bsdiff.h"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include")

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/Findbsdiff.cmake DESTINATION ${CURRENT_PACKAGES_DIR}/share/bsdiff)
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake DESTINATION ${CURRENT_PACKAGES_DIR}/share/bsdiff)
file(INSTALL ${CMAKE_CURRENT_LIST_DIR}/usage DESTINATION ${CURRENT_PACKAGES_DIR}/share/bsdiff)

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
