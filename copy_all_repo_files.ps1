# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $Source, [string] $Destination)

copy "$Source/*" $Destination

function MigrateFolder([string]$source, [string]$destination)
{
    $source = $source.replace("/", "\");
    $destination = $destination.replace("/", "\");
    echo "Calling robocopy.exe $source $destination /e /purge"
    & robocopy.exe $source $destination /e /purge
}

ls . -ad | ForEach-Object -Process { $subfolder = $_.Name; MigrateFolder "$Source/$subfolder" "$Destination/$subfolder" }
