#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

VERSION_PART=$1
VERSION_STYLE=$2

if [ -z "$VERSION_PART" ]
then
	VERSION_PART="semver"
fi

if [ -z "$VERSION_STYLE" ]
then
	VERSION_STYLE="raw"
fi

semver_version=$(cat version.txt)

if [ "$VERSION_PART" == "semver" ]
then
	version_value=${semver_version}
fi

IFS='.' read -ra version_parts <<< ${semver_version}

if [ "$VERSION_PART" == "major" ]
then
	version_value=${version_parts[0]}
fi

if [ "$VERSION_PART" == "minor" ]
then
        version_value=${version_parts[1]}
fi

if [ "$VERSION_PART" == "patch" ]
then
        version_value=${version_parts[2]}
fi

if [ "$VERSION_STYLE" == "raw" ]
then
	echo ${version_value}
fi

if [ "$VERSION_STYLE" == "ado" ]
then
	echo "##vso[task.setvariable variable=${VERSION_PART}_version]${version_value}"
fi
