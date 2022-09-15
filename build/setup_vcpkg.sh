# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
vcpkg_root=$1
port_root=$2
triplet=$3

if [ -z "$port_root" ]
then
    echo Usage:
    echo "    ${0} <vcpkg root> <port root> [<triplet>]"
    echo "    <vcpkg root> Root for installing vcpkg"
    echo "    <port root> Location of vcpkg ports. This is under vcpkg/ports in the ADU Diffs repo"
    echo "    [<triplet>] Optional vcpkg triplet - default is x64-linux"
    exit 1
fi

if [ -z "$triplet" ]
then
    triplet="x64-linux"
fi

echo Configuring VCPKG at $vcpkg_root using ports at $port_root for triplet $triplet

test -d $vcpkg_root || git clone https://github.com/microsoft/vcpkg $vcpkg_root

pushd $vcpkg_root
git pull origin 727bdba9a3765a181cb6be9cb1f6aa01f2c2b5df
git checkout FETCH_HEAD

$vcpkg_root/bootstrap-vcpkg.sh

function vcpkg_install()
{
    $vcpkg_root/vcpkg install $1:$triplet --overlay-ports=$port_root
    exit_code=$?
    if [ $exit_code != 0 ]
    then
        echo "Failed to install $1:$triplet with error $exit_code"
        cat /home/vsts/work/1/vcpkg/buildtrees/ms-gsl/config-x64-mingw-static-dbg-out.log
        cat /home/vsts/work/1/vcpkg/buildtrees/ms-gsl/config-x64-mingw-static-dbg-err.log

        exit $exit_code
    fi
}

if [ -f $vcpkg_root/triplets/$triplet.cmake ]
then
    echo "Found triplet in $vcpkg_root/triplets"
    cat $vcpkg_root/triplets/$triplet.cmake

elif [ -f $vcpkg_root/triplets/community/$triplet.cmake ]
then
    echo "Found triplet in $vcpkg_root/triplets/community"
    cat $vcpkg_root/triplets/community/$triplet.cmake

else
    echo "Couldn't find triplet in triplets or triplets/community!"
fi

if [ $triplet != "x64-mingw-static" ]
then
    vcpkg_install zlib
    vcpkg_install zstd
    vcpkg_install bzip2
    vcpkg_install bsdiff
    vcpkg_install gtest
    vcpkg_install libgpg-error
    vcpkg_install libgcrypt
fi

vcpkg_install ms-gsl
vcpkg_install e2fsprogs
vcpkg_install vcpkg-cmake-config
vcpkg_install vcpkg-cmake

$vcpkg_root/vcpkg integrate install

$vcpkg_root/vcpkg list

cmake_toolchain_file=$vcpkg_root/scripts/buildsystems/vcpkg.cmake
echo CMake arguments should be: -DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file
