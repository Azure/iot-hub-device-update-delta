#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
dumpextfs_src_path="$1"
cmake_output_root="$2"
vcpkg_root="$3"
output_path="$4"

function usage()
{
    echo "Usage: $0 <dumpextfs src path> <cmake output root> <vcpkg root> <output path>"
}

if [ -z "$dumpextfs_src_path" ] | [ -z "$cmake_output_root" ] | [ -z "$vcpkg_root" ] | [ -z "$output_path" ]
then
usage
exit 1
fi

e2fsprogs_vcpkg_root=$vcpkg_root/packages/e2fsprogs_x64-mingw-static
source_files="main.cpp dump_json.cpp load_ext4.cpp file_details.cpp"
e2fsprogs_include="$e2fsprogs_vcpkg_root/include"
e2fsprogs_libraries="$e2fsprogs_vcpkg_root/lib/libext2fs.a $e2fsprogs_vcpkg_root/lib/libcom_err.a"
msgsl_vcpkg_root=$vcpkg_root/packages/ms-gsl_x64-mingw-static
msgsl_include=$msgsl_vcpkg_root/include/
adudiffapi_libraries="$cmake_output_root/hash_utility/libhash_utility.a $cmake_output_root/error_utility/liberror_utility.a"
mingw_win32_libraries="/usr/x86_64-w64-mingw32/lib/libwsock32.a /usr/x86_64-w64-mingw32/lib/libbcrypt.a"
defines="-DWIN32 -DNO_INLINE_FUNCS"
build_flags="-static-libgcc -static-libstdc++ -static -pthread -std=c++17"
includes="-I$msgsl_include -I$e2fsprogs_include -I../../error_utility -I../../hash_utility"
libraries="$e2fsprogs_libraries $adudiffapi_libraries $mingw_win32_libraries"

echo "Attempting to compile code in $dumpextfs_src_path into $output_path using cmake output at $cmake_output_root and vcpkg files at $vcpkg_root"

mkdir -p "$(dirname "$output_path")"
exit_code=$?
if [ $exit_code != 0 ]
then
    echo "Couldn't create directory output for $output_path"
    exit $exit_code
fi

pushd $dumpextfs_src_path
x86_64-w64-mingw32-g++ $includes $source_files $libraries -o $output_path $defines $build_flags
exit_code=$?
if [ $exit_code != 0 ]
then
    echo "Failed to compile with exit code: $exit_code"
    popd
    exit $exit_code
fi
echo "Successfully created binary at $output_path."
ls -l $output_path*
popd
