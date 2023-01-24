# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([Parameter(Mandatory=$true,ValueFromPipeline=$false)][string] $Source, [Parameter(Mandatory=$true,ValueFromPipeline=$false)][string] $Destination)

& robocopy.exe $Source $Destination /e /purge /xd .git
