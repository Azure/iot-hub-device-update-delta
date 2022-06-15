# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $repoRoot, [string] $vcpkgRoot, [string] $buildType)

# prereqs:
#  Windows Environment: Visual Studio installation with msbuild in path
#  WSL environment setup with g++-9 g++-mingw-w64-i686 g++-mingw-w64-x86-64 g++-mingw-w64 g++ mingw-w64-common mingw-w64-i686-dev mingw-w64-x86-64-dev mingw-w64

echo "Repo is located at $repoRoot"
echo "Using VCPKG at $vcpkgRoot"

# Setup VCPKG
$portRoot="$repoRoot\vcpkg\ports"
$triplet="x64-windows"

if ([string]::IsNullOrEmpty($buildType))
{
    $buildType="Release"
}

echo "Build type is: $buildType"

echo "Configuring VCPKG at $vcpkgRoot using ports at $portRoot for triplet $triplet"

if (-Not (Test-Path -Path $vcpkgRoot))
{
    & git clone https://github.com/microsoft/vcpkg $vcpkgRoot
}

& $vcpkgRoot\bootstrap-vcpkg.bat

& $vcpkgRoot\vcpkg.exe install zlib:$triplet --overlay-ports=$portRoot
& $vcpkgRoot\vcpkg.exe install zstd:$triplet --overlay-ports=$portRoot
& $vcpkgRoot\vcpkg.exe install ms-gsl:$triplet --overlay-ports=$portRoot
& $vcpkgRoot\vcpkg.exe install gtest:$triplet --overlay-ports=$portRoot
& $vcpkgRoot\vcpkg.exe install bsdiff:$triplet --overlay-ports=$portRoot

& $vcpkgRoot\vcpkg.exe integrate install

$cmakeBuildDir="$repoRoot\src\out"
$vcpkgToolchainFile="$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"

if (Test-Path -Path $cmakeBuildDir)
{
#    rm -Force -Recurse $cmakeBuildDir
}

$cmakeArgs ='-S ' + $repoRoot + '\src -B ' + $cmakeBuildDir + ' -DCMAKE_TOOLCHAIN_FILE="' + ${vcpkgToolchainFile} +'" -DCMAKE_BUILD_TYPE="' + ${buildType} + '"'
echo "Executing cmake with args $cmakeArgs"
& cmake -S $repoRoot\src -B $cmakeBuildDir -DCMAKE_TOOLCHAIN_FILE="$vcpkgToolchainFile" -DCMAKE_BUILD_TYPE="$buildType"
$cmakeSolution="$cmakeBuildDir\adu_diffs.sln"
& msbuild $cmakeSolution /property:DebugSymbols=true /property:Configuration=$buildType

$wslPathVcpkgRoot = & wsl wslpath $vcpkgRoot.replace("\", "\\")
$wslRepoRoot = & wsl wslpath $repoRoot.replace("\", "\\")

& wsl $wslPathVcpkgRoot/bootstrap-vcpkg.sh
& wsl $wslPathVcpkgRoot/vcpkg install e2fsprogs:x64-mingw-static --overlay-ports="$wslRepoRoot/vcpkg/ports"
& wsl --cd $wslRepoRoot/src/tools/dumpextfs ./build_vcpkg.win32 $wslPathVcpkgRoot/packages/e2fsprogs_x64-mingw-static
