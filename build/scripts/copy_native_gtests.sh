#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
cmake_build_dir=$1
destination=$2

echo "Copying tests under $cmake_build_dir to $destination"

mkdir -p $destination > /dev/null

ALL_GTESTS=$(find $cmake_build_dir -name "*gtest" -type f)

for f in $ALL_GTESTS
do
	echo "Copying $f to $destination"
	cp $f $destination
done

echo "$destination contents: "
ls -b $destination
