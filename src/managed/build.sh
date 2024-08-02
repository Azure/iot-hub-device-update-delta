#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
CONFIGURATION=$1

if [ -z "$CONFIGURATION" ]; then
    CONFIGURATION="Debug"
fi

dotnet build ./DiffGen/diff-generation.sln --configuration $CONFIGURATION