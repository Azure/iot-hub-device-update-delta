# !/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
INSTALL_VCPKG_ROOT=$1
PORT_ROOT=$2
TRIPLET=$3

if [ -z "$PORT_ROOT" ]
then
    echo Usage:
    echo "    ${0} <vcpkg root> <port root> [<TRIPLET>]"
    echo "    <vcpkg root> Root for installing vcpkg"
    echo "    <port root> Location of vcpkg ports. This is under vcpkg/ports in the ADU Diffs repo"
    echo "    [<TRIPLET>] Optional vcpkg TRIPLET - default is x64-linux"
    exit 1
fi

if [ -z "$TRIPLET" ]
then
    TRIPLET="x64-linux"
fi

if [ -z "$VCPKG_ROOT" ]
then
    echo "No VCPKG_ROOT environment variable specified. Using $INSTALL_VCPKG_ROOT."
    VCPKG_ROOT=$INSTALL_VCPKG_ROOT
else
    echo "vcpkg root specified. Using $VCPKG_ROOT and ignoring user input."
fi

echo Configuring VCPKG at $VCPKG_ROOT using ports at $PORT_ROOT for TRIPLET $TRIPLET

# If we have a directory setup already, use it.
if [ ! -d "$VCPKG_ROOT" ]
then
	git clone https://github.com/microsoft/vcpkg $VCPKG_ROOT
	exit_code=$?
	if [ $exit_code != 0 ]
	then
		echo "Failed to clone vcpkg."
		exit $exit_code
	fi
	echo "exit_code: $exit_code"

    pushd $VCPKG_ROOT
    git pull
    popd
fi

$VCPKG_ROOT/bootstrap-vcpkg.sh

if [ -f $VCPKG_ROOT/TRIPLETs/$TRIPLET.cmake ]
then
    echo "Found TRIPLET in $VCPKG_ROOT/TRIPLETs"
    cat $VCPKG_ROOT/TRIPLETs/$TRIPLET.cmake

elif [ -f $VCPKG_ROOT/TRIPLETs/community/$TRIPLET.cmake ]
then
    echo "Found TRIPLET in $VCPKG_ROOT/TRIPLETs/community"
    cat $VCPKG_ROOT/TRIPLETs/community/$TRIPLET.cmake

else
    echo "Couldn't find $TRIPLET in TRIPLETs or TRIPLETs/community!"
fi

function vcpkg_install()
{
	echo "Calling $VCPKG_ROOT/vcpkg install $1:$TRIPLET --overlay-ports=$PORT_ROOT"
    $VCPKG_ROOT/vcpkg install $1:$TRIPLET --overlay-ports=$PORT_ROOT
    exit_code=$?
    if [ $exit_code != 0 ]
    then
        echo "Failed to install $1:$TRIPLET with error $exit_code"
        exit $exit_code
    fi
}

vcpkg_install zlib
vcpkg_install zstd
vcpkg_install bzip2
vcpkg_install bsdiff
vcpkg_install gtest
vcpkg_install openssl
vcpkg_install e2fsprogs
vcpkg_install vcpkg-cmake-config
vcpkg_install vcpkg-cmake
vcpkg_install jsoncpp
vcpkg_install libconfig
vcpkg_install fmt

$VCPKG_ROOT/vcpkg integrate install

$VCPKG_ROOT/vcpkg list

cmake_toolchain_file=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
echo CMake arguments should be: -DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file
