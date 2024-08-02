# !/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
CONFIGURATION=$1
PLATFORM=$2

if [ -z "$CONFIGURATION" ]
then
    CONFIGURATION="Debug"
    echo "Setting CONFIGURATION to '${CONFIGURATION}"
fi

if [ -z "$PLATFORM" ]
then
    PLATFORM="x64"
    echo "Setting PLATFORM to '${PLATFORM}"
fi

Usage() {
    echo "$0 [Debug|Release] [x64|arm|arm64]"
}

VCPKG_TRIPLET="${PLATFORM}-linux"

if [ "$CONFIGURATION" != "Release" ] && [ "$CONFIGURATION" != "Debug" ] ; then
    echo "Invalid platform: $CONFIGURATION. Valid options: Release, Debug"
    Usage
    exit 1
fi

if [ "$PLATFORM" != "x64" ] && [ "$PLATFORM" != "arm" ] && [ "$PLATFORM" != "arm64" ] ; then
    echo "Invalid platform: $PLATFORM. Valid options: x64, arm, arm64"
    Usage
    exit 1
fi

if [ "$PLATFORM" == "arm" ] ; then
    QEMU_COMMAND="qemu-arm -L /usr/arm-linux-gnueabihf "
elif [ "$PLATFORM" == "arm64" ] ; then
    QEMU_COMMAND="qemu-aarch64 -L /usr/aarch64-linux-gnu "
else
    VALGRIND_COMMAND="valgrind --leak-check=full --show-leak-kinds=all "
fi

echo "CONFIGURATION: $CONFIGURATION"
echo "PLATFORM: $PLATFORM"

SCRIPT_DIR=$( realpath -e $(dirname $0) )
chmod +x $SCRIPT_DIR/GetCMakeBuildDir.sh
CMAKE_BUILD_DIR=$( $SCRIPT_DIR/GetCMakeBuildDir.sh $VCPKG_TRIPLET $CONFIGURATION )
echo "CMAKE_BUILD_DIR: $CMAKE_BUILD_DIR"

GetTestExePath() {
    TEST_EXE=$1
    TEST_EXE_PATH=$( realpath -e "${CMAKE_BUILD_DIR}/test/bin/${TEST_EXE}" )
    echo $TEST_EXE_PATH
}

TEST_DATA_PATH=$( realpath -e "../../data" )

declare -i FAILED_TEST_COUNT=0

RunTest() {
    TEST_EXE=$1
    USE_TEST_DATA=$2

    TEST_EXE_PATH=$( GetTestExePath ${TEST_EXE} )

    TEST_COMMAND="${VALGRIND_COMMAND}${QEMU_COMMAND}${TEST_EXE_PATH}"

    if [ ! -z "$USE_TEST_DATA" ] ; then
        TEST_COMMAND="${TEST_COMMAND} --test_data_root $TEST_DATA_PATH"
    fi

    echo "Calling: ${TEST_COMMAND}"
    if $TEST_COMMAND ; then
        echo "Test Passed."
    else
        echo "Test Failed."
	FAILED_TEST_COUNT=$((FAILED_TEST_COUNT + 1))
    fi
}

RunTest io_gtest
RunTest io_buffer_gtest
RunTest io_hashed_gtest
RunTest io_file_gtest
RunTest io_compressed_gtest true
RunTest cpio_archives_gtest

RunTest diffs_core_gtest
RunTest diffs_recipes_basic_gtest
RunTest diffs_recipes_compressed_gtest true

if [ "${FAILED_TEST_COUNT}" == "0" ] ; then
    echo "All tests passed!"
else
    echo "${FAILED_TEST_COUNT} tests failed."
    exit 1
fi
