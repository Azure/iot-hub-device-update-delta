# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $vcpkgRoot, [string] $overlayRoot, [string] $triplet)

if ([string]::IsNullOrEmpty($vcpkgRoot)) {
    Write-Output "Usage: setup_vcpkg.ps1 <vcpkgRoot> <overlayRoot> [triplet]"
    exit 1
}

if ([string]::IsNullOrEmpty($triplet)) {
    $triplet = "x64-windows"
}

Write-Output "vcpkgRoot: $vcpkgRoot"
Write-Output "overlayRoot: $overlayRoot"
Write-Output "triplet: $triplet"

$portRoot = "$overlayRoot/ports"
$tripletsRoot = "$overlayRoot/triplets"

Write-Output "portRoot: $portRoot"
Write-Output "tripletsRoot: $tripletsRoot"

if (Test-Path env:VCPKG_ROOT) {
    Write-Output "Found vcpkg root at $env:VCPKG_ROOT"
    $vcpkgRoot = $env:VCPKG_ROOT
}

if (Test-Path "$vcpkgRoot\.git") {
    Push-Location $vcpkgRoot
    git pull
    Pop-Location
}
else {
    git clone https://github.com/microsoft/vcpkg $vcpkgRoot
}

function BootstrapVcpkg() {
    & $vcpkgRoot/bootstrap-vcpkg.bat
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function InstallToVcpkg([string] $packageName) {
    Write-Output "Calling: & $vcpkgRoot/vcpkg.exe install ${packageName}:$triplet --overlay-ports=$portRoot --overlay-triplets=$tripletsRoot"
    & $vcpkgRoot/vcpkg.exe install ${packageName}:$triplet --overlay-ports=$portRoot --overlay-triplets=$tripletsRoot
    if ($LASTEXITCODE -ne 0) {
        Write-Output "Failed to setup: $packageName"
        exit $LASTEXITCODE
    }

    Write-Output "Successfully setup: $packageName to $vcpkgRoot"
}

function IntegrateInstall() {
    & $vcpkgRoot/vcpkg.exe integrate install
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function ListPackages() {
    & $vcpkgRoot/vcpkg.exe list
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

BootstrapVcpkg

InstallToVcpkg "zlib"
InstallToVcpkg "zstd"
InstallToVcpkg "gtest"
InstallToVcpkg "bzip2"
InstallToVcpkg "bsdiff"
InstallToVcpkg "e2fsprogs"
InstallToVcpkg "jsoncpp"
InstallToVcpkg "libconfig"
InstallToVcpkg "fmt"

IntegrateInstall
ListPackages