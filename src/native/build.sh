#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
SCRIPT_DIR=$( realpath -e $(dirname $0) )
echo "Script dir: ${SCRIPT_DIR}"

Usage() {
    echo "./build.sh [x64-linux|arm64-linux|arm64-linux] [Release|Debug] [vcpkg|cmake|build|all]"
}

VCPKG_TRIPLET=$1

if [ -z "$VCPKG_TRIPLET" ]
then
    echo "No triplet specified. Using 'x64-linux' triplet."
    VCPKG_TRIPLET="x64-linux"
fi

if [ "$VCPKG_TRIPLET" != "x64-linux" ] && [ "$VCPKG_TRIPLET" != "arm-linux" ] && [ "$VCPKG_TRIPLET" != "arm64-linux" ]
then
    echo "Invalid triplet: $VCPKG_TRIPLET. Valid options: x64-linux, arm-linux, arm64-linux"
    Usage
    exit 1
fi

BUILD_TYPE=$2

if [ -z "$BUILD_TYPE" ]
then
    BUILD_TYPE="Debug"
    echo "No build type specified specified. Using '$BUILD_TYPE'."
fi

if [ "$BUILD_TYPE" != "Release" ] && [ "$BUILD_TYPE" != "Debug" ]
then
    echo "Invalid build type: $BUILD_TYPE. Valid options: Release, Debug"
    Usage
    exit 1
fi

STAGES=$3

if [ -z "$STAGES" ]
then
    echo "No stage specified. Using 'all'."
    STAGES="all"
fi

if [ "$STAGES" != "vcpkg" ] && [ "$STAGES" != "cmake" ] && [ "$STAGES" != "build" ] && [ "$STAGES" != "all" ]
then
    echo "Invalid stages: $STAGES. Valid options: vcpkg, cmake, build and all"
    Usage
    exit 1
fi

echo "VCPKG_TRIPLET  : $VCPKG_TRIPLET"
echo "BUILD_TYPE     : $BUILD_TYPE"
echo "STAGES         : $STAGES"

CMAKE_SOURCE_DIR=$SCRIPT_DIR

chmod +x $SCRIPT_DIR/GetCMakeBuildDir.sh
CMAKE_BUILD_DIR=$( $SCRIPT_DIR/GetCMakeBuildDir.sh $VCPKG_TRIPLET $BUILD_TYPE )
echo "CMAKE_BUILD_DIR: $CMAKE_BUILD_DIR"

THIS_REPO_ROOT_DIR=$( realpath -e "${SCRIPT_DIR}/../.." )

echo "THIS_REPO_ROOT_DIR: ${THIS_REPO_ROOT_DIR}"

VCPKG_TOOLS_DIR="${THIS_REPO_ROOT_DIR}/vcpkg"
echo "VCPKG_TOOLS_DIR: ${VCPKG_TOOLS_DIR}"

if [ -z "$VCPKG_ROOT" ] ; then
    VCPKG_REPO_ROOT_DIR=$( realpath -m "${THIS_REPO_ROOT_DIR}/../vcpkg" )
    if [ -z "$VCPKG_REPO_ROOT_DIR" ] ; then
        echo "Couldn't determine VCPKG_REPO_ROOT_DIR."
        exit 1
    fi
else
    VCPKG_REPO_ROOT_DIR=$VCPKG_ROOT
fi

echo "VCPKG_REPO_ROOT_DIR: ${VCPKG_REPO_ROOT_DIR}"

SetupVcpkg() {

    echo "Setting up VCPKG..."

    chmod +x ${VCPKG_TOOLS_DIR}/setup_vcpkg.sh

    if ${VCPKG_TOOLS_DIR}/setup_vcpkg.sh ${VCPKG_REPO_ROOT_DIR} ${VCPKG_TOOLS_DIR}/ports ${VCPKG_TRIPLET} ; then
        echo "Finished setting up vcpkg."
    else
        echo "VCPKG failed."
        exit 1
    fi
}

if [ "$VCPKG_TRIPLET" == "x64-linux" ] ; then
    C_COMPILER="/usr/bin/gcc"
    CXX_COMPILER="/usr/bin/g++"
    PACKAGE_TARGET_ARCHITECTURE="x86_64"
elif [ "$VCPKG_TRIPLET" == "arm-linux" ] ; then
    C_COMPILER="/usr/bin/arm-linux-gnueabihf-gcc"
    CXX_COMPILER="/usr/bin/arm-linux-gnueabihf-g++"
    PACKAGE_TARGET_ARCHITECTURE="arm"
elif [ "$VCPKG_TRIPLET" == "arm64-linux" ] ; then
    C_COMPILER="/usr/bin/aarch64-linux-gnu-gcc"
    CXX_COMPILER="/usr/bin/aarch64-linux-gnu-g++"
    PACKAGE_TARGET_ARCHITECTURE="arm64"
fi

RunCMake() {
    if [ ! -d "$CMAKE_BUILD_DIR" ]; then
        echo "Creating $CMAKE_BUILD_DIR"
        mkdir -p $CMAKE_BUILD_DIR
    fi

    VCPKG_TOOLCHAIN_FILE=${VCPKG_REPO_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake

    echo "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}"
    echo "CMAKE_BUILD_DIR: ${CMAKE_BUILD_DIR}"
    echo "Using toolchain file: ${VCPKG_TOOLCHAIN_FILE}"
    CMAKE_GENERATOR="Unix Makefiles"
    echo "Using Generator: ${CMAKE_GENERATOR}"

    CMAKE_CACHE_FILE="${CMAKE_BUILD_DIR}/CMakeCache.txt"
    if [ -f $CMAKE_CACHE_FILE ]; then
        echo "Deleting existing CMake Cache at: $CMAKE_CACHE_FILE"
        rm $CMAKE_CACHE_FILE
    fi

    echo "Using C Compiler: ${C_COMPILER}"
    echo "Using C++ Compiler: ${CXX_COMPILER}"

    pushd ../..
    chmod +x get_version.sh
    ADU_DIFFS_VERSION=$(./get_version.sh)
    popd

    echo "ADU_DIFFS_VERSION: ${ADU_DIFFS_VERSION}"

    if cmake -S $CMAKE_SOURCE_DIR -B $CMAKE_BUILD_DIR -G "${CMAKE_GENERATOR}" -DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN_FILE} -DVCPKG_TARGET_TRIPLET=${VCPKG_TRIPLET} -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_C_COMPILER=${C_COMPILER} -DCMAKE_CXX_COMPILER=${CXX_COMPILER} -DPACKAGE_TARGET_ARCHITECTURE=${PACKAGE_TARGET_ARCHITECTURE} -DADU_DIFFS_VERSION=${ADU_DIFFS_VERSION} ; then
        echo "Finished setting up build folders with cmake."
    else
        echo "Cmake failed."
        exit 1
    fi
}

CopyBsdiffBinaryFiles() {
    if [ "$BUILD_TYPE" == "Release" ] ; then
        BSDIFF_PACKAGE_BINARIES=$( realpath -m "${VCPKG_REPO_ROOT_DIR}/packages/bsdiff_${VCPKG_TRIPLET}/bin" )
    fi

    if [ "$BUILD_TYPE" == "Debug" ] ; then
        BSDIFF_PACKAGE_BINARIES=$( realpath -m "${VCPKG_REPO_ROOT_DIR}/packages/bsdiff_${VCPKG_TRIPLET}/debug/bin" )
    fi

    BIN_TARGET=$( realpath -m "${CMAKE_BUILD_DIR}/bin" )

    if [ -f "$BIN_TARGET/bsdiff" ] ; then
        rm $BIN_TARGET/bsdiff
    fi

    if [ -f "$BIN_TARGET/bspatch" ] ; then
        rm $BIN_TARGET/bspatch
    fi

    cp $BSDIFF_PACKAGE_BINARIES/bsdiff_diff $BIN_TARGET/bsdiff
    cp $BSDIFF_PACKAGE_BINARIES/bsdiff_patch $BIN_TARGET/bspatch
}

BASE_LICENSE_SOURCE="$THIS_REPO_ROOT_DIR/licenses/LICENSE.linux"
BASE_LICENSE_TARGET="$BIN_TARGET/NOTICE"

function CopyBaseLinuxLicense {
	cp $BASE_LICENSE_SOURCE $BASE_LICENSE_TARGET
}

function CopyVcpkgLicenseFile {
    PACKAGE_NAME=$1
    LICENSE_FILE=$2

    if [[ -z "$LICENSE_FILE" ]] ; then
        LICENSE_FILE="copyright"
    fi

    COPYRIGHT_FILE=$( realpath -e "$VCPKG_REPO_ROOT_DIR/packages/${PACKAGE_NAME}_${VCPKG_TRIPLET}/share/${PACKAGE_NAME}/${LICENSE_FILE}" )
    cp $COPYRIGHT_FILE $BIN_TARGET/LICENSE.${PACKAGE_NAME}

    echo "================ License for $PACKAGE_NAME ================" >> $BASE_LICENSE_TARGET
    cat $COPYRIGHT_FILE >> $BASE_LICENSE_TARGET
}

Build() {
    pushd $CMAKE_BUILD_DIR
    if make ; then
        echo "Make succeeded!"
    else
        echo "Make failed!"
        popd
        exit 1
    fi
    popd

    echo "Copying VCPKG binaries and license files."
    CopyBsdiffBinaryFiles

    CopyBaseLinuxLicense

    CopyVcpkgLicenseFile bsdiff
    CopyVcpkgLicenseFile bzip2
    CopyVcpkgLicenseFile e2fsprogs
    CopyVcpkgLicenseFile jsoncpp
    CopyVcpkgLicenseFile libconfig
    CopyVcpkgLicenseFile zlib
    CopyVcpkgLicenseFile zstd LICENSE
    CopyVcpkgLicenseFile openssl

    echo "Binaries ($CMAKE_BUILD_DIR):"
    ls $CMAKE_BUILD_DIR/bin

    echo "Test Binaries ($CMAKE_BUILD_DIR):"
    ls $CMAKE_BUILD_DIR/test/bin
}

case $STAGES in
    vcpkg)
        SetupVcpkg
    ;;
    cmake)
        RunCMake
    ;;
    build)
        Build
    ;;
    all)
        SetupVcpkg
        RunCMake
        Build
    ;;
esac

