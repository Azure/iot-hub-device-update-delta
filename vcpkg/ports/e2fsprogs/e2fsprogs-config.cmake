# - Find e2fsprogs
# Find the ext2fs library
# This module defines
#  E2FSPROGS_INCLUDE_DIRS, where to find e2fsprogs libraries
# E2FSPROGS_LIBRARIES, the libraries needed to use e2fsprogs
#

find_path(E2FSPROGS_INCLUDE_DIRS
    NAMES com_err.h bitops.h ext2_err.h ext2_ext_attr.h ext2_fs.h ext2_io.h ext2_types.h ext2fs.h ext3_extents.h hashmap.h qcow2.h tdb.h
    PATHS ${CMAKE_CURRENT_LIST_DIR}/../../include/et ${CMAKE_CURRENT_LIST_DIR}/../../include/ext2fs
)
mark_as_advanced(E2FSPROGS_INCLUDE_DIRS)

include(SelectLibraryConfigurations)

find_library(COM_ERR_LIBRARY_RELEASE NAMES com_err PATH ${CMAKE_CURRENT_LIST_DIR}/../../lib)
find_library(COM_ERR_LIBRARY_DEBUG NAMES com_err PATHS ${CMAKE_CURRENT_LIST_DIR}/../../debug/lib)
select_library_configurations(COM_ERR)

find_library(EXT2FS_LIBRARY_RELEASE NAMES ext2fs PATH ${CMAKE_CURRENT_LIST_DIR}/../../lib)
find_library(EXT2FS_LIBRARY_DEBUG NAMES ext2fs PATHS ${CMAKE_CURRENT_LIST_DIR}/../../debug/lib)
select_library_configurations(EXT2FS)

set(E2FSPROGS_LIBRARIES ${EXT2FS_LIBRARY} ${COM_ERR_LIBRARY})
mark_as_advanced(E2FSPROGS_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(E2FSPROGS DEFAULT_MSG E2FSPROGS_INCLUDE_DIRS E2FSPROGS_LIBRARIES)

