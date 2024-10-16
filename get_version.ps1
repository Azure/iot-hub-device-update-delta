# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
param([string] $VersionPart, [string] $VersionStyle)

if ([string]::IsNullOrEmpty($VersionPart)) {
	$VersionPart = "semver"
}

if ([string]::IsNullOrEmpty($VersionStyle)) {
	$VersionStyle = "raw"
}

$semVerValue = Get-Content version.txt

if ($VersionPart -match "semver") {
	$versionValue = $semVerValue
}

$version_parts= $($semVerValue -split '.')

if ($VersionPart -match "major") {
	$versionValue = $version_parts[0]
}

if ($VersionPart -match "minor") {
	$versionValue = $version_parts[1]
}

if ($VersionPart -match "patch") {
	$versionValue = $version_parts[2]
}

if ($VersionStyle -match "raw") {
	Write-Output $versionValue
}

if ($VersionStyle -match "ado") {
	Write-Output "##vso[task.setvariable variable=${VersionPart}_version]${versionValue}"
}
