# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $vcpkgRoot, [string] $repoRoot, [string] $triplet)

if ([string]::IsNullOrEmpty($vcpkgRoot))
{
    echo "Usage: setup_vcpkg.ps1 <vcpkgRoot> <repoRoot> [triplet]"
    exit 1
}

if ([string]::IsNullOrEmpty($triplet))
{
    $triplet="x64-windows"
}

git clone https://github.com/microsoft/vcpkg $vcpkgRoot

$portRoot="$repoRoot/vcpkg/ports"

function BootstrapVcpkg()
{
    & $vcpkgRoot/bootstrap-vcpkg.bat
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}

function InstallToVcpkg([string] $packageName)
{
    & $vcpkgRoot/vcpkg.exe install ${packageName}:$triplet --overlay-ports=$portRoot
    if ($LASTEXITCODE -ne 0)
    {
        echo "Failed to setup: $packageName"
        exit $LASTEXITCODE
    }

    echo "Successfully setup: $packageName"
}

function IntegrateInstall()
{
    & $vcpkgRoot/vcpkg.exe integrate install
    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}

BootstrapVcpkg

InstallToVcpkg "zlib"
InstallToVcpkg "zstd"
InstallToVcpkg "ms-gsl"
InstallToVcpkg "gtest"
InstallToVcpkg "bzip2"
InstallToVcpkg "bsdiff"

IntegrateInstall