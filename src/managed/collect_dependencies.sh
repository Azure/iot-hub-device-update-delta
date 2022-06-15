#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

#example: ./collect_dependencies.sh /mnt/c/code/AzureDeviceUpdateDiffs/src/managed/DiffGen/tests/UnitTests/bin/Debug/net5.0
destination=$1

gitDir=$(git rev-parse --git-dir)
dumpextfs_path=$(realpath "$gitDir/../src/tools/dumpextfs/dumpextfs")
zstd_compress_file_path=$(realpath "$gitDir/../src/out/tools/zstd_compress_file/zstd_compress_file")
libadudiffapi_path=$(realpath "$gitDir/../src/out/diffs/api/libadudiffapi.so")

cp $dumpextfs_path $destination
cp $zstd_compress_file_path $destination
cp $libadudiffapi_path $destination
