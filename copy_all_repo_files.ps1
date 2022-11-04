param(
    [Parameter(Mandatory=$True, Position=0, ValueFromPipeline=$false)]
    [System.String]
    $Source,

    [Parameter(Mandatory=$True, Position=1, ValueFromPipeline=$false)]
    [System.String]
    $Destination
)

copy "$Source/*" $Destination

function MigrateFolder([string]$source, [string]$destination)
{
    $source = $source.replace("/", "\");
    $destination = $destination.replace("/", "\");
    echo "Calling robocopy.exe $source $destination /e /purge"
    & robocopy.exe $source $destination /e /purge
}

ls . -ad | ForEach-Object -Process { $subfolder = $_.Name; MigrateFolder "$Source/$subfolder" "$Destination/$subfolder" }
