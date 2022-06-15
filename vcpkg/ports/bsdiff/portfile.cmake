vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO zhuyie/bsdiff
    REF d668332fa50fe55e74442dbee9e75acf26e40801
    SHA512 6d6f71ffc5bc56891e14655365b5118f9e3460ac160cc5d179814d841899f9bc45500788ea2e7478930085097a98023fde8cf9c11389ad33d5e27f8c462bef59
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
