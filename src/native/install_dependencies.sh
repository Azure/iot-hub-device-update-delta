#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

VCPKG_TRIPLET=$1

if [ -z "$VCPKG_TRIPLET" ]
then
VCPKG_TRIPLET="x64-linux"
echo "VCPKG_TRIPLET not specified. Using '${VCPKG_TRIPLET}'"
fi

if [ "$VCPKG_TRIPLET" != "x64-linux" ] && [ "$VCPKG_TRIPLET" != "arm-linux" ] && [ "$VCPKG_TRIPLET" != "arm64-linux" ]
then
    echo "Invalid triplet: $VCPKG_TRIPLET. Valid options: x64-linux, arm-linux, arm64-linux"
    exit 1
fi

COMMON_DEPENDENCIES="gcc gcc-10 g++ g++-10 autoconf autopoint ninja-build tree"
QEMU_COMMON_DEPENDENCIES="qemu-utils qemu-user"

if [ "$VCPKG_TRIPLET" == "x64-linux" ] ; then
	DEPENDENCIES="${COMMON_DEPENDENCIES} valgrind"
	USR_BIN_GCC="/usr/bin/gcc"
	GCC_COMMAND="gcc"
	C_COMPILER="/usr/bin/gcc-10"
	USR_BIN_GXX="/usr/bin/g++"
	GXX_COMMAND="g++"
	CXX_COMPILER="/usr/bin/g++-10"
elif [ "$VCPKG_TRIPLET" == "arm-linux" ] ; then
	DEPENDENCIES="${COMMON_DEPENDENCIES} ${QEMU_COMMON_DEPENDENCIES} gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf g++-10-arm-linux-gnueabihf binutils-arm-linux-gnueabihf qemu-system-arm "
	USR_BIN_GCC="/usr/bin/arm-linux-gnueabihf-gcc"
	GCC_COMMAND="arm-linux-gnueabihf-gcc"
	C_COMPILER="/usr/bin/arm-linux-gnueabihf-gcc-10"
	USR_BIN_GXX="/usr/bin/arm-linux-gnueabihf-g++"
	GXX_COMMAND="arm-linux-gnueabihf-g++"
	CXX_COMPILER="/usr/bin/arm-linux-gnueabihf-g++-10"
elif [ "$VCPKG_TRIPLET" == "arm64-linux" ] ; then
	DEPENDENCIES="${COMMON_DEPENDENCIES} ${QEMU_COMMON_DEPENDENCIES} gcc-aarch64-linux-gnu g++-aarch64-linux-gnu binutils-aarch64-linux-gnu g++-10-aarch64-linux-gnu qemu-system-arm qemu-efi-aarch64"
	USR_BIN_GCC="/usr/bin/aarch64-linux-gnu-gcc"
	GCC_COMMAND="aarch64-linux-gnu-gcc"
	C_COMPILER="/usr/bin/aarch64-linux-gnu-gcc-10"
	USR_BIN_GXX="/usr/bin/aarch64-linux-gnu-g++"
	GXX_COMMAND="aarch64-linux-gnu-g++"
	CXX_COMPILER="/usr/bin/aarch64-linux-gnu-g++-10"
fi
echo "Installing dependencies: ${DEPENDENCIES}"

sudo apt-get -y update
sudo apt-get -y upgrade
sudo apt-get -y install ${DEPENDENCIES}

echo "C_COMPILER: ${C_COMPILER}"
echo "CXX_COMPILER: ${CXX_COMPILER}"

echo "Updating alternatives. ${GCC_COMMAND} -> ${C_COMPILER}, ${GXX_COMMAND} -> ${CXX_COMPILER}"
UPDATE_ALTERNATIVE_GCC="sudo update-alternatives --install ${USR_BIN_GCC} ${GCC_COMMAND} ${C_COMPILER} 10"
echo "Calling: $UPDATE_ALTERNATIVE_GCC"
$UPDATE_ALTERNATIVE_GCC
UPDATE_ALTERNATIVE_GXX="sudo update-alternatives --install ${USR_BIN_GXX} ${GXX_COMMAND} ${CXX_COMPILER} 10"
echo "Calling: $UPDATE_ALTERNATIVE_GXX"
$UPDATE_ALTERNATIVE_GXX
