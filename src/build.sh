# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
pushd hash_utility
make $@
popd
pushd diffs
make $@
pushd api
make $@
popd
popd
pushd tools
pushd applydiff
make $@
popd
pushd zstd_compress_file
make $@
popd
popd
