#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# prereqs: libmhash-dev, g++-9, autopoint, autoconf

repo_root="$1"
vcpkg_root="$2"
triplet="$3"

if [ -z "$triplet" ]
then
triplet="x64-linux"
fi

if [ "$triplet" == "x64-linux" ]
then
    cmake_c_compiler=/usr/bin/gcc-9
    cmake_cxx_compiler=/usr/bin/g++-9
elif [ "$triplet" == "arm-linux" ]
then
    cmake_c_compiler=/usr/bin/arm-linux-gnueabihf-gcc-9
    cmake_cxx_compiler=/usr/bin/arm-linux-gnueabihf-g++-9
elif [ "$triplet" == "x64-linux" ]
then
    cmake_c_compiler=/usr/bin/aarch64-linux-gnu-gcc-9
    cmake_cxx_compiler=/usr/bin/aarch64-linux-gnu-g++-9
else
    echo "Invalid triplet specified."
    exit 1
fi

echo "Repo is located at ${repo_root}"
echo "Using VCPKG at $vcpkg_root"

# Setup VCPKG
port_root="$repo_root/vcpkg/ports"

echo "Configuring VCPKG at $vcpkg_root using ports at $port_root for triplet $triplet"

test -d $vcpkg_root || git clone https://github.com/microsoft/vcpkg $vcpkg_root
pushd $vcpkg_root
git pull origin 727bdba9a3765a181cb6be9cb1f6aa01f2c2b5df
git checkout FETCH_HEAD
popd

$vcpkg_root/bootstrap-vcpkg.sh

$vcpkg_root/vcpkg install zlib:$triplet --overlay-ports=$port_root
$vcpkg_root/vcpkg install zstd:$triplet --overlay-ports=$port_root
$vcpkg_root/vcpkg install ms-gsl:$triplet --overlay-ports=$port_root
$vcpkg_root/vcpkg install gtest:$triplet --overlay-ports=$port_root
$vcpkg_root/vcpkg install bsdiff:$triplet --overlay-ports=$port_root
$vcpkg_root/vcpkg install libgcrypt:$triplet --overlay-ports=$port_root

$vcpkg_root/vcpkg integrate install

cmake_build_dir="$repo_root/src/out.$triplet"
vcpkg_toolchain_file="$vcpkg_root/scripts/buildsystems/vcpkg.cmake"

rm -rf $cmake_build_dir

cmake -S $repo_root/src -B $cmake_build_dir -DCMAKE_TOOLCHAIN_FILE="$vcpkg_toolchain_file" -GNinja -DCMAKE_C_COMPILER=$cmake_c_compiler -DCMAKE_CXX_COMPILER=$cmake_cxx_compiler

cd $cmake_build_dir && ninja


if [ "$triplet" == "x64-linux" ]
then
    $vcpkg_root/vcpkg install e2fsprogs:x64-linux --overlay-ports="$port_root"
    $vcpkg_root/vcpkg integrate install

    cd $repo_root/src/tools/dumpextfs
    ./build_vcpkg $vcpkg_root/packages/e2fsprogs_x64-linux
fi
