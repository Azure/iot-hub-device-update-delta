# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $cmakeBuildDir, [string] $vcpkgRoot, [string] $destination, [string] $buildType)

echo "Copying native binaries."
echo " cmakeBuildDir: $cmakeBuildDir"
echo " vcpkgRoot: $vcpkgRoot"
echo " buildType: $buildType"
echo " destination: $destination"

if ([string]::IsNullOrEmpty($buildType)) {
    $buildType = "Release"
}

$vsBuildTypeSubFolder = "Release"
$zstdDllName = "zstd.dll"
$zlibDllName = "zlib1.dll"
$bz2DllName = "bz2.dll"
$vcpkgBinFolder = "bin"

if ($buildType.Contains("Debug")) {
    $vsBuildTypeSubFolder = "Debug"
    $zstdDllName = "zstdd.dll"
    $zlibDllName = "zlibd1.dll"
    $bz2DllName = "bz2d.dll"
    $vcpkgBinFolder = "debug/bin"
}
else {
    Write-Output "Invalid build type: $buildType"
    exit 1
}

if ([string]::IsNullOrEmpty($cmakeBuildDir) -or [string]::IsNullOrEmpty($vcpkgRoot) -or [string]::IsNullOrEmpty($destination)) {
    Write-Output "Usage: $PSCommandPath <cmake build dir> <vcpkg root> <destination> [build type]"
    exit 1
}

$cmakeBuildDir = $cmakeBuildDir.Replace("/", "\")
$vcpkgRoot = $vcpkgRoot.Replace("/", "\")
$destination = $destination.Replace("/", "\")

if (!(Test-Path $destination)) {
    New-Item -Path $destination -ItemType Directory
}

Get-ChildItem $vcpkgRoot\packages -Recurse -Filter "*.exe"
Get-ChildItem $vcpkgRoot\packages -Recurse -Filter "*.dll"

Copy-Item -Path $vcpkgRoot\packages\bsdiff_x64-windows\$vcpkgBinFolder\bsdiff_diff.exe -Destination $destination\bsdiff.exe
Copy-Item -Path $vcpkgRoot\packages\bsdiff_x64-windows\$vcpkgBinFolder\bsdiff_patch.exe -Destination $destination\bspatch.exe
Copy-Item -Path $vcpkgRoot\packages\bsdiff_x64-windows\$vcpkgBinFolder\bsdiff.dll -Destination $destination\bsdiff.dll
Copy-Item -Path $vcpkgRoot\packages\bsdiff_x64-windows\share\bsdiff\copyright -Destination $destination\LICENSE.bsdiff
Copy-Item -Path $vcpkgRoot\packages\bzip2_x64-windows\share\bzip2\copyright -Destination $destination\LICENSE.bzip2
Copy-Item -Path $vcpkgRoot\packages\bzip2_x64-windows\$vcpkgBinFolder\$bz2DllName -Destination $destination\$bz2DllName
Copy-Item -Path $vcpkgRoot\packages\jsoncpp_x64-windows\share\jsoncpp\copyright -Destination $destination\LICENSE.jsoncpp
Copy-Item -Path $vcpkgRoot\packages\jsoncpp_x64-windows\$vcpkgBinFolder\jsoncpp.dll -Destination $destination\jsoncpp.dll

Copy-Item -Path $vcpkgRoot\packages\zlib_x64-windows\$vcpkgBinFolder\$zlibDllName -Destination $destination\$zlibDllName
Copy-Item -Path $vcpkgRoot\packages\zlib_x64-windows\share\zlib\copyright -Destination $destination\LICENSE.zlib

Copy-Item -Path $vcpkgRoot\packages\zstd_x64-windows\$vcpkgBinFolder\$zstdDllName -Destination $destination\$zstdDllName
Copy-Item -Path $vcpkgRoot\packages\zstd_x64-windows\share\zstd\copyright -Destination $destination\LICENSE.zstd

Copy-Item  -Path $vcpkgRoot\packages\e2fsprogs_x64-windows\share\e2fsprogs\copyright $destination/LICENSE.e2fsprogs

Get-ChildItem $cmakeBuildDir\diffs -Recurse -Filter "adudiffapi.*"

$vsBuildTypeSubFolder = ""

Copy-Item -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.dll -Destination $destination\adudiffapi.dll
Copy-Item -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.lib -Destination $destination\adudiffapi.lib
Copy-Item -Path $cmakeBuildDir\diffs\api\$vsBuildTypeSubFolder\adudiffapi.pdb -Destination $destination\adudiffapi.pdb

Get-ChildItem $cmakeBuildDir\tools -Recurse -Filter "*.exe"

Copy-Item -Path $cmakeBuildDir\tools\zstd_compress_file\$vsBuildTypeSubFolder\zstd_compress_file.exe -Destination $destination\zstd_compress_file.exe
Copy-Item -Path $cmakeBuildDir\tools\zstd_compress_file\$vsBuildTypeSubFolder\zstd_compress_file.pdb -Destination $destination\zstd_compress_file.pdb
Copy-Item -Path $cmakeBuildDir\tools\applydiff\$vsBuildTypeSubFolder\applydiff.exe -Destination $destination\applydiff.exe
Copy-Item -Path $cmakeBuildDir\tools\applydiff\$vsBuildTypeSubFolder\applydiff.pdb -Destination $destination\applydiff.pdb
Copy-Item -Path $cmakeBuildDir\tools\dumpdiff\$vsBuildTypeSubFolder\dumpdiff.exe -Destination $destination\dumpdiff.exe
Copy-Item -Path $cmakeBuildDir\tools\dumpdiff\$vsBuildTypeSubFolder\dumpdiff.pdb -Destination $destination\dumpdiff.pdb
Copy-Item -Path $cmakeBuildDir\tools\dumpextfs\$vsBuildTypeSubFolder\dumpextfs.exe -Destination $destination\dumpextfs.exe
Copy-Item -Path $cmakeBuildDir\tools\dumpextfs\$vsBuildTypeSubFolder\dumpextfs.pdb -Destination $destination\dumpextfs.pdb

Get-ChildItem $destination