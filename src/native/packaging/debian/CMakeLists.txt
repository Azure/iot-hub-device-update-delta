set(CPACK_PACKAGE_NAME "ms-${PROJECT_NAME}" CACHE STRING "deviceupdate-deltaprocessor")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Device Update Delta Processor")
set(CPACK_PACKAGE_VENDOR "Microsoft")
set(CPACK_PACKAGE_CONTACT "adudev@microsoft.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ADU Developers")

# -dumpmachine gives the target triple
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMD_ARGS "-target" "${CMAKE_C_COMPILER_TARGET}" "-dumpmachine")
else()
    set(CMD_ARGS "-dumpmachine")
endif()
execute_process(
    COMMAND
        ${CMAKE_C_COMPILER} ${CMD_ARGS}
    RESULT_VARIABLE
        RESULT
    OUTPUT_VARIABLE
        ARCH
    ERROR_QUIET)
if(RESULT)
    message(FATAL_ERROR "Failed to determine target architecture triplet: ${RESULT}")
endif()
string(REGEX MATCH "([^-]+).*" ARCH_MATCH ${ARCH})
if(NOT CMAKE_MATCH_1 OR NOT ARCH_MATCH)
    message(FATAL_ERROR "Failed to match the target architecture triplet: ${ARCH}")
endif()
set(ARCH ${CMAKE_MATCH_1})

if("${ARCH}" STREQUAL "x86_64")
    set(ARCH "amd64")
elseif("${ARCH}" STREQUAL "aarch64")
    set(ARCH "arm64")
elseif("${ARCH}" STREQUAL "i686")
    # Do nothing
elseif("${ARCH}" STREQUAL "i386")
    # Do nothing
elseif("${ARCH}" STREQUAL "arm")
else()
    message(FATAL_ERROR "Unrecognized architecture ${ARCH} from ${CMAKE_C_COMPILER}")
endif()

set(PACKAGE_TARGET_ARCHITECTURE ${ARCH} CACHE STRING "Target architecture for the package")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${ARCH})

set(CPACK_VERBATIM_VARIABLES YES)

# Debian
# See https://cmake.org/cmake/help/v3.3/module/CPackDeb.html for debian specific CPack variables.

if (NOT CPACK_GENERATOR)
    set (CPACK_GENERATOR "DEB")
else ()
    string (APPEND CPACK_GENERATOR ";DEB")
endif ()

set (CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Azure/iot-hub-device-update-delta")
set (CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
set (CPACK_DEBIAN_PACKAGE_SECTION "admin")

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/_packages")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE, ${PACKAGE_TARGET_ARCHITECTURE})

set(CPACK_DEBIAN_FILE_NAME "${CPACK_PACKAGE_NAME}_${PROJECT_VERSION}_${PACKAGE_TARGET_ARCHITECTURE}.deb")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION ${CPACK_PACKAGE_NAME})

set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_DEB_COMPONENTS_INSTALL YES)

include(CPack)
