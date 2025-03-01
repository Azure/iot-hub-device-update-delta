#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# Get the input directory from the command line argument
InputDirectory="$1"

# Change directory to the parent of the parent directory
pushd ../.. > /dev/null
ADU_DIFFS_VERSION=$(./get_version.sh)
popd > /dev/null

echo "Updating VERSION_STRING to $ADU_DIFFS_VERSION for all files in $InputDirectory"

# Find all files in the input directory
files=$(find "$InputDirectory" -type f)

for file in $files; do
    newFile=$(echo "$file" | sed "s/VERSION_STRING/$ADU_DIFFS_VERSION/g")
    if [[ "$file" != "$newFile" ]]; then
        echo "Moving $file to $newFile"
        mv "$file" "$newFile"
    else
        echo "Not changing file: $file"
    fi
done