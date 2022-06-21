# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $cmakeBuildDir, [string] $vcpkgRoot, [string] $destination, [string] $buildType)

if ([string]::IsNullOrEmpty($buildType))
{
    $buildType="Release"
}

$vcpkgBuildTypeSubFolder=""
$vsBuildTypeSubFolder=""
$zstdDllName=""
$zlibDllName=""

if ($buildType.Contains("Release"))
{
    $vcpkgBuildTypeSubFolder="."
    $vsBuildTypeSubFolder="Release"
    $zstdDllName="zstd.dll"
    $zlibDllName="zlib1.dll"
}
elseif ($buildType.Contains("Debug"))
{
    $vcpkgBuildTypeSubFolder = "debug"
    $vsBuildTypeSubFolder="Debug"
    $zstdDllName="zstdd.dll"
    $zlibDllName="zlibd1.dll"
}
else
{
    echo "Invalid build type: $buildType"
    exit 1
}

if ([string]::IsNullOrEmpty($cmakeBuildDir) -or [string]::IsNullOrEmpty($vcpkgRoot) -or [string]::IsNullOrEmpty($destination))
{
    echo "Usage: $PSCommandPath <cmake build dir> <vcpkg root> <destination> [build type]"
    exit 1
}

$cmakeBuildDir=$cmakeBuildDir.Replace("/", "\")
$vcpkgRoot=$vcpkgRoot.Replace("/", "\")
$destination=$destination.Replace("/", "\")

echo "Copying binaries built to $cmakeBuildDir with vcpkg at $vcpkgRoot and buildType: $buildType to $destination"

copy -Path $vcpkgRoot\packages\bsdiff_x64-windows\$vcpkgBuildTypeSubFolder\bin\bsdiff_diff.exe -Destination $destination\bsdiff.exe
copy -Path $vcpkgRoot\packages\bsdiff_x64-windows\$vcpkgBuildTypeSubFolder\bin\bsdiff_patch.exe -Destination $destination\bspatch.exe
copy -Path $vcpkgRoot\packages\bsdiff_x64-windows\bin\bsdiff.dll -Destination $destination\bsdiff.dll
copy -Path $vcpkgRoot\packages\bsdiff_x64-windows\share\bsdiff\copyright -Destination $destination\LICENSE.bsdiff
copy -Path $vcpkgRoot\packages\bzip2_x64-windows\share\bzip2\copyright -Destination $destination\LICENSE.bzip2
copy -Path $vcpkgRoot\packages\bzip2_x64-windows\bin\bz2.dll -Destination $destination\bz2.dll

copy -Path $vcpkgRoot\packages\zlib_x64-windows\$vcpkgBuildTypeSubFolder\bin\$zlibDllName -Destination $destination\$zlibDllName
copy -Path $vcpkgRoot\packages\zlib_x64-windows\share\zlib\copyright -Destination $destination\LICENSE.zlib

copy -Path $vcpkgRoot\packages\zstd_x64-windows\$vcpkgBuildTypeSubFolder\bin\$zstdDllName -Destination $destination\$zstdDllName
copy -Path $vcpkgRoot\packages\zstd_x64-windows\share\zstd\copyright -Destination $destination\LICENSE.zstd

copy -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.dll -Destination $destination\adudiffapi.dll
copy -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.lib -Destination $destination\adudiffapi.lib
copy -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.pdb -Destination $destination\adudiffapi.pdb

copy -Path $cmakeBuildDir\tools\zstd_compress_file\$vsBuildTypeSubFolder\zstd_compress_file.exe -Destination $destination\zstd_compress_file.exe

copy -Path $cmakeBuildDir\tools\applydiff\$vsBuildTypeSubFolder\applydiff.exe -Destination $destination\applydiff.exe
copy -Path $cmakeBuildDir\tools\dumpdiff\$vsBuildTypeSubFolder\dumpdiff.exe -Destination $destination\dumpdiff.exe

ls $destination