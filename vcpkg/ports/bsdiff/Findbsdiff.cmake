# - Find BSDIFF
# Find the BSDIFF library 
# This module defines
#  BSDIFF_INCLUDE_DIRS, where to find bsdiff.h
# BSDIFF_LIBRARIES, the libraries needed to use BSDIFF
#

find_path(BSDIFF_INCLUDE_DIRS
    NAMES bsdiff.h
    PATH_SUFFIXES bsdiff
)
mark_as_advanced(BSDIFF_INCLUDE_DIRS)

include(SelectLibraryConfigurations)

find_library(BSDIFF_LIBRARY_RELEASE NAMES bsdiff PATH "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
find_library(BSDIFF_LIBRARY_DEBUG NAMES bsdiff PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
select_library_configurations(BSDIFF)

find_library(DIVSUFSORT64_LIBRARY_RELEASE NAMES divsufsort64 PATH "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
find_library(DIVSUFSORT64_LIBRARY_DEBUG NAMES divsufsort64 PATHS $"${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
select_library_configurations(DIVSUFSORT64)

find_library(DIVSUFSORT_LIBRARY_RELEASE NAMES divsufsort PATH "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
find_library(DIVSUFSORT_LIBRARY_DEBUG NAMES divsufsort PATHS $"${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib")
select_library_configurations(DIVSUFSORT)

find_package(BZip2)

set(BSDIFF_LIBRARIES ${BSDIFF_LIBRARY} ${BZIP2_LIBRARIES} ${DIVSUFSORT_LIBRARY} ${DIVSUFSORT64_LIBRARY})
mark_as_advanced(BSDIFF_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BSDIFF DEFAULT_MSG BSDIFF_INCLUDE_DIRS BSDIFF_LIBRARIES)
