#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
SCRIPT_DIR=$( realpath -e $(dirname $0) )
echo "Script dir: ${SCRIPT_DIR}"

Usage() {
    echo "./build.sh [x64-linux|arm64-linux|arm64-linux] [Release|Debug]"
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

pushd native
./build.sh $VCPKG_TRIPLET $BUILD_TYPE
popd

pushd managed
./build.sh $BUILD_TYPE
popd