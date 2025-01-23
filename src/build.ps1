# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
param([string] $VcpkgTriplet, [string] $BuildType)

function Usage {
	Write-Host "Usage: build.ps1 [vcpkg_triplet] [build_type]"
	Write-Host "   vcpkg_triplet: 'x64-windows' (default) or 'x86-windows'"
	Write-Host "   build_type   : 'Debug' (default) or 'Release'"
}

if ([string]::IsNullOrEmpty($VcpkgTriplet)) {
	$VcpkgTriplet = "x64-windows"
}

if (-not (($VcpkgTriplet -match "x64-windows") -or ($VcpkgTriplet -match "x86-windows"))) {
	Write-Host "Invalid VcpkgTriplet: $VcpkgTriplet"
	Usage
	exit 1
}

if ([string]::IsNullOrEmpty($BuildType)) {
	$BuildType = "Debug"
}

if (-not ($BuildType -match "Release") -and -not ($BuildType -match "Debug")) {
	Write-Host "Invalid BuildType: $BuildType"
	Usage
	exit 1
}

Push-Location native
./build.ps1 $VcpkgTriplet $BuildType "all"
Pop-Location
if ($LASTEXITCODE -ne 0) {
	Write-Host "Result from native build was $LASTEXITCODE, exiting."
	exit $LASTEXITCODE
}

Push-Location managed
./build.ps1 $BuildType
Pop-Location
if ($LASTEXITCODE -ne 0) {
	Write-Host "Result from managed build was $LASTEXITCODE, exiting."
	exit $LASTEXITCODE
}
