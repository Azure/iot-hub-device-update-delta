# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $root)

echo "Getting net6.0 dll versions for root: $root"

Get-ChildItem $root -recurse |
    where {$_.directory -like '*net6.0' -and $_.name -like '*.dll' -and $_.name.Split('.').Length -eq 2} |
    Select FullName |
    ForEach-Object { 
        $fullName = $_.FullName;
        $fileVersion = (Get-Item $fullName).VersionInfo.FileVersion;
        $productInfo = (Get-Item $fullName).VersionInfo.ProductVersion;
        $copyright = (Get-Item $fullName).VersionInfo.LegalCopyright;
        echo "$fullName. File Version: $fileVersion, Product Info: $productInfo, LegalCopyright: $copyright";
        }
