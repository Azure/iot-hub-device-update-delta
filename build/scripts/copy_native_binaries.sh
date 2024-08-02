#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
cmake_build_dir=$1
vcpkg_root=$2
destination=$3

mkdir -p $destination > /dev/null

cp $vcpkg_root/packages/bsdiff_x64-linux/share/bsdiff/copyright $destination/LICENSE.bsdiff
cp $vcpkg_root/packages/bzip2_x64-linux/share/bzip2/copyright $destination/LICENSE.bzip2
cp $vcpkg_root/packages/jsoncpp_x64-linux/share/jsoncpp/copyright $destination/LICENSE.jsoncpp

cp $vcpkg_root/packages/zlib_x64-linux/share/zlib/copyright $destination/LICENSE.zlib

cp $vcpkg_root/packages/zstd_x64-linux/share/zstd/copyright $destination/LICENSE.zstd

cp $vcpkg_root/packages/e2fsprogs_x64-linux/share/e2fsprogs/copyright $destination/LICENSE.e2fsprogs

cp $cmake_build_dir/diffs/api/libadudiffapi.so $destination/libadudiffapi.so

cp $cmake_build_dir/tools/zstd_compress_file/zstd_compress_file $destination/zstd_compress_file
cp $cmake_build_dir/tools/applydiff/applydiff $destination/applydiff
cp $cmake_build_dir/tools/dumpdiff/dumpdiff $destination/dumpdiff
cp $cmake_build_dir/tools/dumpextfs/dumpextfs $destination/dumpextfs

echo "Copied files to $destination: "
ls -b $destination