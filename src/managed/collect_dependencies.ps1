# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#example usage: .\collect_dependencies.ps1 C:\code\vcpkg C:\code\adu2\src\managed\DiffGen\tests\UnitTests\bin\debug\net5.0\
param(
    [Parameter()]
    [string]$vcpkg_root,
    [Parameter()]
    [string]$destination
)

function Git-Dir
{
    return git rev-parse --git-dir
}

function Combine([string[]] $a)
{
    [IO.Path]::Combine([string[]] (@($a[0]) + $a[1..$($a.count-1)] -replace '^\\', ''))
}

$cmake_output_root = Combine($(Git-Dir), '..\src\out\build\x64-Debug\')

if (-not (Test-Path -Path $destination))
{
    mkdir $destination
}

$diffsapi_dll_path = Combine($cmake_output_root, 'diffs\api\adudiffapi.dll')
copy $diffsapi_dll_path -Destination $destination

$zstd_compress_file_path = Combine($cmake_output_root, 'tools\zstd_compress_file\zstd_compress_file.exe')
copy $zstd_compress_file_path -Destination $destination

$dumpextfs_path = Combine($(Git-Dir), '..\src\tools\dumpextfs\dumpextfs.exe')
copy $dumpextfs_path -Destination $destination

$zlib_path = Combine($vcpkg_root, 'packages\zlib_x64-windows\debug\bin\zlibd1.dll')
copy $zlib_path -Destination $destination

$zstd_path = Combine($vcpkg_root, 'packages\zstd_x64-windows\debug\bin\zstdd.dll')
copy $zstd_path -Destination $destination

$bsdiff_path = Combine($vcpkg_root, 'packages\bsdiff_x64-windows-static\bin\bsdiff_diff.exe')
$bsdiff_destination_path = Combine($destination, 'bsdiff.exe')
copy $bsdiff_path $bsdiff_destination_path

$bspatch_path = Combine($vcpkg_root, 'packages\bsdiff_x64-windows-static\bin\bsdiff_patch.exe')
$bspatch_destination_path = Combine($destination, 'bspatch.exe')
copy $bspatch_path $bspatch_destination_path
