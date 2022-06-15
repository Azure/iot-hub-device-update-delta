#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
repo_root=$1
vcpkg_root=$2
destination=$3

mkdir -p $destination > /dev/null

cmake_build_dir="$repo_root/src/out.linux"

cp $vcpkg_root/packages/bsdiff_x64-linux/share/bsdiff/copyright $destination/LICENSE.bsdiff
cp $vcpkg_root/packages/bzip2_x64-linux/share/bzip2/copyright $destination/LICENSE.bzip2

cp $vcpkg_root/packages/zlib_x64-linux/share/zlib/copyright $destination/LICENSE.zlib

cp $vcpkg_root/packages/zstd_x64-linux/share/zstd/copyright $destination/LICENSE.zstd

cp $vcpkg_root/packages/e2fsprogs_x64-linux/share/e2fsprogs/copyright $destination/LICENSE.e2fsprogs

cp $repo_root/src/tools/dumpextfs/dumpextfs $destination/dumpextfs

cp $cmake_build_dir/diffs/api/libadudiffapi.so $destination/libadudiffapi.so

cp $cmake_build_dir/tools/zstd_compress_file/zstd_compress_file $destination/zstd_compress_file

echo "Copied files to $destination: "
ls -b $destination