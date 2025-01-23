# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $vcpkgRoot, [string] $repoRoot, [string] $triplet)

if ([string]::IsNullOrEmpty($vcpkgRoot)) {
    Write-Host "Usage: setup_vcpkg.ps1 <vcpkgRoot> <repoRoot> [triplet]"
    exit 1
}

if ([string]::IsNullOrEmpty($triplet)) {
    $triplet = "x64-windows"
}

Write-Host "vcpkgRoot: $vcpkgRoot"
Write-Host "repoRoot: $repoRoot"
Write-Host "triplet: $triplet"

$repoVcpkgRoot = "$repoRoot/vcpkg"

$portRoot = "$repoVcpkgRoot/ports"
$tripletsRoot = "$repoVcpkgRoot/triplets"

Write-Host "portRoot: $portRoot"
Write-Host "tripletsRoot: $tripletsRoot"

if ($null -ne $env:VCPKG_ROOT) {
    $vcpkgRoot = $env:VCPKG_ROOT
}
else {
    if (-not (Test-Path "$vcpkgRoot\.git")) {
        git clone https://github.com/microsoft/vcpkg $vcpkgRoot | Out-Host
    }

    Push-Location $vcpkgRoot
        $mostRecentVcpkgTag=git describe --tags --abbrev=0
        git checkout $mostRecentVcpkgTag | Out-Host
    Pop-Location
}

function SetupTerrapin() {
    Write-Host "Setting up Terrapin"
    if ($null -eq (Get-Command "nuget.exe" -ErrorAction SilentlyContinue)) {
        Write-Host "Unable to find nuget.exe. To download: https://learn.microsoft.com/en-us/nuget/reference/nuget-exe-cli-reference?tabs=windows#installing-nugetexe"
        exit 1
    }

    $nugetPackagesDirectory = Join-Path $env:temp "adu.vcpkg\packages"
    $nugetConfig = Join-Path $repoRoot "nuget.config"
    $packagesConfig = Join-Path $repoVcpkgRoot "packages.config"

    nuget restore $packagesConfig -ConfigFile $nugetConfig -PackagesDirectory $nugetPackagesDirectory | Out-Host
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $win64Directory = (Get-ChildItem $nugetPackagesDirectory\TerrapinRetrievalTool* -Include win-x64 -Attributes Directory -Recurse)
    if ($null -eq $win64Directory) {
        Write-Host "Failed to find TerrapinRetrievalTool win-x64 directory"
        exit 1
    }

    $terrapinTool = "$win64Directory\TerrapinRetrievalTool.exe"

    Write-Host "terrapinTool $terrapinTool"

    $env:X_VCPKG_ASSET_SOURCES = "x-script,`"$terrapinTool`" -b https://vcpkg.storage.devpackages.microsoft.io/artifacts/ -k https://terraprodvcpkgstoracc.blob.core.windows.net/artifacts/ -a true -p {url} -s {sha512} -d {dst};x-block-origin"

    Write-Host "Set X_VCPKG_ASSET_SOURCES env variable to: $env:X_VCPKG_ASSET_SOURCES"
}

function BootstrapVcpkg() {
    & $vcpkgRoot/bootstrap-vcpkg.bat | Out-Host
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function InstallToVcpkg([string] $packageName) {
    Write-Host "Calling: & $vcpkgRoot/vcpkg.exe install ${packageName}:$triplet --overlay-ports=$portRoot --overlay-triplets=$tripletsRoot"
    & $vcpkgRoot/vcpkg.exe install ${packageName}:$triplet --overlay-ports=$portRoot --overlay-triplets=$tripletsRoot | Out-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to setup: $packageName"
        exit $LASTEXITCODE
    }

    Write-Host "Successfully setup: $packageName to $vcpkgRoot"
}

function IntegrateInstall() {
    & $vcpkgRoot/vcpkg.exe integrate install | Out-Host
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

function ListPackages() {
    & $vcpkgRoot/vcpkg.exe list | Out-Host
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

#SetupTerrapin

if ($null -eq $env:VCPKG_ROOT) {
    BootstrapVcpkg
}
else {
    Write-OutpuHost "Skipping VCPKG bootstrap as VCPKG_ROOT is already set."
}

InstallToVcpkg "zlib"
InstallToVcpkg "zstd"
InstallToVcpkg "gtest"
InstallToVcpkg "bzip2"
InstallToVcpkg "bsdiff"
InstallToVcpkg "e2fsprogs"
InstallToVcpkg "jsoncpp"
InstallToVcpkg "libconfig"
InstallToVcpkg "fmt"
InstallToVcpkg "openssl"

IntegrateInstall
ListPackages