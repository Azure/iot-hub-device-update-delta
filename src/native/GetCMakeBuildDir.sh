#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
SCRIPT_DIR=$( dirname $0 )
CMAKE_BUILD_DIR=$( realpath -m "${SCRIPT_DIR}/../out/native/$1/$2" )
mkdir -p ${CMAKE_BUILD_DIR} > /dev/null
echo "${CMAKE_BUILD_DIR}"
