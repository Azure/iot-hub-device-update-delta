# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
param([string] $VcpkgTriplet, [string] $BuildType, [string] $Stages)

function Usage {
	Write-Host "Usage: build.ps1 [vcpkg_triplet] [build_type] [stages]"
	Write-Host "   vcpkg_triplet: 'x64-windows' (default) or 'x86-windows'"
	Write-Host "   build_type   : 'Debug' (default) or 'Release'"
	Write-Host "   stages       : 'vcpkg', 'cmake', 'build', 'all' (default)"
}

if ([string]::IsNullOrEmpty($VcpkgTriplet)) {
	$VcpkgTriplet = "x64-windows"
}

if ($VcpkgTriplet -match "x64-windows") {
	$cmakePlatformParameter = "-A x64"
}
elseif ($VcpkgTriplet -match "x86-windows") {
	$cmakePlatformParameter = "-DCMAKE_GENERATOR_PLATFORM=WIN32"
}
else {
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

if ([string]::IsNullOrEmpty($Stages)) {
	$Stages = "all"
}


Write-Host "VcpkgTriplet    : $VcpkgTriplet"
Write-Host "BuildType       : $BuildType"
Write-Host "Stages          : $Stages"

# Put our vcpkg repo as a sibling to this repo. We are at <adu delta repo>\src\native.
$vcpkgRepo = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath("..\..\..\vcpkg")

Write-Host "Vcpkg Repo: $vcpkgRepo"

function RunSetupVCPkg {
	Write-Output "Setting up VCPKG repo in ${vcpkgRepo}"

	$setupVCPkgScript = Resolve-Path("..\..\vcpkg\setup_vcpkg.ps1")
	$vcpkgOverlayRoot = Resolve-Path("..\..\vcpkg")

	Write-Output "Calling ${setupVCPkgScript} $vcpkgRepo $vcpkgOverlayRoot $VcpkgTriplet"

	&${setupVCPkgScript} $vcpkgRepo $vcpkgOverlayRoot $VcpkgTriplet
	if ($LASTEXITCODE -ne 0) {
		Write-Output "VCPKG Setup failed."
		exit $LASTEXITCODE
	}
}

function FindToolInPath {
	param (
		[string] $Root,
		[string] $ToolName
	)

	if (( ${env:path}.Split(";") | Where-Object { Test-Path -Path "$_\$ToolName" } | Measure-Object ).Count -ne 0) {
		Write-Host "$ToolName is available via path environment."

		return $ToolName
	}

	$possiblePaths = (Get-ChildItem -Path "$Root" -Recurse -Filter "$ToolName" -ErrorAction SilentlyContinue)
	$entryCounts = ($possiblePaths | Measure-Object).Count

	if ($entryCounts -eq 0) {
		Write-Host "Couldn't find a path for $ToolName in $Root"
		exit 1
	}

	$toolDir = $possiblePaths[0].Directory.FullName
	$toolDir = $toolDir.Replace('\', '/')

	if (!$toolDir.EndsWith('/')) {

		$toolDir = $toolDir + '/'
	}

	return "$toolDir$ToolName"
}

$cmakeSourceDir = Resolve-Path(".")
$cmakeBuildDir = & .\GetCMakeBuildDir $VcpkgTriplet $BuildType
Write-Host "CMake Source Dir: $cmakeSourceDir"
Write-Host "CMake Build Dir : $cmakeBuildDir"

function RunCMake {
	$cmake = FindToolInPath $env:ProgramFiles "cmake.exe"

	$vcpkgCMakeToolChainFile = Resolve-Path("${vcpkgRepo}\scripts\buildsystems\vcpkg.cmake")

	$vcpkgCMakeCache = "$cmakeBuildDir\CMakeCache.txt"

	if (Test-Path -Path $vcpkgCMakeCache) {
		Write-Host "Removing CMake Cache file: $vcpkgCMakeCache"
		Remove-Item $vcpkgCMakeCache
	}

	Write-Host "Calling: `"$cmake`" -S $cmakeSourceDir -B $cmakeBuildDir $cmakePlatformParameter -DCMAKE_TOOLCHAIN_FILE=`"$vcpkgCMakeToolChainFile`" -DCMAKE_BUILD_TYPE=`"$BuildType`" -DVCPKG_TARGET_TRIPLET=`"$VcpkgTriplet`""
	& "$cmake" -S $cmakeSourceDir -B $cmakeBuildDir $cmakePlatformParameter -DCMAKE_TOOLCHAIN_FILE="$vcpkgCMakeToolChainFile" -DCMAKE_BUILD_TYPE="$BuildType" -DVCPKG_TARGET_TRIPLET="$VcpkgTriplet"

	if ($LASTEXITCODE -ne 0) {
		Write-Output "CMake failed."
		exit $LASTEXITCODE
	}
}

function CopyBsdiffBinaryFiles {
	if ($BuildType -match "Release") {
		$bsdiffPackageBinaries = Resolve-Path("${vcpkgRepo}/packages/bsdiff_${VcpkgTriplet}/bin")
	}

	if ($BuildType -match "Debug") {
		$bsdiffPackageBinaries = Resolve-Path("${vcpkgRepo}/packages/bsdiff_${VcpkgTriplet}/debug/bin")
	}

	$targetBin = "${cmakeBuildDir}\bin\$BuildType"

	Copy-Item $bsdiffPackageBinaries/* ${cmakeBuildDir}\bin\$BuildType
	Move-Item $targetBin/bsdiff_diff.exe $targetBin/bsdiff.exe -Force
	Move-Item $targetBin/bsdiff_patch.exe $targetBin/bspatch.exe -Force
}

function CopyVcpkgLicenseFile {
	param([string]$PackageName)

	$targetBin = "${cmakeBuildDir}\bin\$BuildType"

	$copyrightFile = Resolve-Path("$vcpkgRepo/packages/${PackageName}_${VcpkgTriplet}/share/${PackageName}/copyright")
	Copy-Item $copyrightFile $targetBin/LICENSE.${PackageName}
}

function RunMsBuild {
	$msbuild = FindToolInPath $env:ProgramFiles "msbuild.exe"

	Write-Host "Calling: `"${msbuild}`" $cmakeBuildDir\adu_diffs.sln /property:OutputPath=. /property:UseStructuredOutput=false /property:Configuration=$BuildType"
	& "$msbuild" $cmakeBuildDir\adu_diffs.sln /property:OutputPath=. /property:UseStructuredOutput=false /property:Configuration=$BuildType

	if ($LASTEXITCODE -ne 0) {
		Write-Output "Build failed."
		exit $LASTEXITCODE
	}

	Write-Output "Copying VCPKG binaries and license files."
	CopyBsdiffBinaryFiles

	CopyVcpkgLicenseFile bsdiff
	CopyVcpkgLicenseFile bzip2
	CopyVcpkgLicenseFile e2fsprogs
	CopyVcpkgLicenseFile jsoncpp
	CopyVcpkgLicenseFile libconfig
	CopyVcpkgLicenseFile zlib
	CopyVcpkgLicenseFile zstd

	Write-Output "Build Output (binaries): " (Get-ChildItem ${cmakeBuildDir}\bin\$BuildType) | Out-Host

	Write-Output "Build Output (test binaries): " (Get-ChildItem ${cmakeBuildDir}\test\bin\$BuildType) | Out-Host
}

switch ($Stages) {
	"vcpkg" {
		RunSetupVCPkg
	}
	"cmake" {
		RunCMake
	}
	"build" {
		RunMsBuild
	}
	"all" {
		RunSetupVCPkg
		RunCmake
		RunMsBuild
	}
	default {
		Write-Host "Invalid Stages value: $Stages"
		Usage
		Exit 1
	}
}