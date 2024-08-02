#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
SCRIPT_DIR=$( dirname $0 )
VCPKG_BUILD_DIR=$( realpath -m "${SCRIPT_DIR}/out.linux/build/$1/$2" )
mkdir -p ${VCPKG_BUILD_DIR} > /dev/null
echo "${VCPKG_BUILD_DIR}"
