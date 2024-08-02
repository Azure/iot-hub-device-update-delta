param ([string] $cmakeBuildDir, [string] $destination)

Write-Host "Copying tests under $cmakeBuildDir to $destination"

if (-not (Test-Path $destination))
{
	New-Item -Path $destination -ItemType Directory
}

$allGtests = (Get-ChildItem -Path $cmakeBuildDir -Recurse -Filter '*gtest.exe' -ErrorAction SilentlyContinue | % { $_.FullName })

foreach ($gtest in $allGtests)
{
	$fullName = $gtest
	Write-Host "Copying $fullName to $destination"
	try
	{
		Copy-Item $fullName -Destination "$destination"
	}
	catch
	{
		Write-Host "Copying $fullName to $destination failed"
		exit 1
	}
}
