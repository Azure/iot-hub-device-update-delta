# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

echo "Making drop at $1."

rm -f -r $1
mkdir $1
mkdir $1/amd64
cp diffs/api/amd64/output/libadudiffexpand.so $1/amd64
cp tools/applydiff/amd64/output/applydiff $1/amd64

mkdir $1/arm64
cp diffs/api/arm64/output/libadudiffexpand.so $1/arm64

mkdir -p $1/src/applydiff
cp tools/applydiff/Makefile $1/src/applydiff
cp tools/applydiff/applydiff.c $1/src/applydiff

mkdir -p $1/src/inc
cp diffs/api/adudiffapply.h $1/src/inc
