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

find_library(BSDIFF_LIBRARY_RELEASE NAMES bsdiff PATH_SUFFIXES lib)
find_library(BSDIFF_LIBRARY_DEBUG NAMES bsdiff PATH_SUFFIXES lib)
select_library_configurations(BSDIFF)

find_library(DIVSUFSORT_LIBRARY_RELEASE NAMES divsufsort64 PATH_SUFFIXES lib)
find_library(DIVSUFSORT_LIBRARY_DEBUG NAMES divsufsort64 PATH_SUFFIXES lib)
select_library_configurations(DIVSUFSORT)

find_package(BZip2)

set(BSDIFF_LIBRARIES ${DIVSUFSORT_LIBRARY} ${BSDIFF_LIBRARY} ${BZIP2_LIBRARIES})
mark_as_advanced(BSDIFF_LIBRARIES)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BSDIFF DEFAULT_MSG BSDIFF_INCLUDE_DIRS BSDIFF_LIBRARIES)
