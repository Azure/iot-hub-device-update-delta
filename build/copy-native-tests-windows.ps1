# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $cmakeBuildDir, [string] $vcpkgRoot, [string] $destination, [string] $buildType)

if ([string]::IsNullOrEmpty($buildType))
{
    $buildType="Release"
}

if ($buildType.Contains("Release"))
{
    $vsBuildTypeSubFolder="Release"
}
elseif ($buildType.Contains("Debug"))
{
    $vsBuildTypeSubFolder="Debug"
}
else
{
    echo "Invalid build type: $buildType"
    exit 1
}

copy -Path $cmakeBuildDir\diffs\gtest\$vsBuildTypeSubFolder\* -Destination $destination
copy -Path $cmakeBuildDir\io_utility\gtest\$vsBuildTypeSubFolder\* -Destination $destination

ls $destination