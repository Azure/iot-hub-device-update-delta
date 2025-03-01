# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
param([string] $InputDirectory)

Push-Location ../..
$ADU_DIFFS_VERSION = $(./get_version.ps1)
Pop-Location

Write-Host "Updating VERSION_STRING to $ADU_DIFFS_VERSION for all files in $InputDirectory"

$files = Get-ChildItem -Path $InputDirectory
$files | ForEach-Object {
    $file = $_.FullName
    $newFile = $file.Replace("VERSION_STRING", $ADU_DIFFS_VERSION)
    if ($file -ne $newFile) {
        Write-Host "Moving $file to $newFile"
        Move-Item $file $newFile
    } else {
        Write-Host "Not changing file: $file"
    }
}