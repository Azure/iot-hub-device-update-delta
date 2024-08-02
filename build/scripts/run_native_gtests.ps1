param ([string] $cmakeBuildDir, [string] $testDataRoot)

Write-Host "Running tests under $cmakeBuildDir using test data root: $testDataRoot"

$allGtests = (Get-ChildItem -Path $cmakeBuildDir -Recurse -Filter '*gtest.exe' -ErrorAction SilentlyContinue | % { $_.FullName } )
$failedGtests = @{}

foreach ($gtest in $allGtests)
{
	Write-Host "Running: $gtest --test_data_root ${testDataRoot}"
	& $gtest --test_data_root ${testDataRoot}

	if ($LASTEXITCODE -ne 0)
	{
		Write-Host "gtest failed with code: $LASTEXITCODE"
		$failedGtests.add(${gtest}, $LASTEXITCODE)
	}
}

if ($failedGtests.count -ne 0)
{
	Write-Host "Failed executing tests:"
	foreach ($failedTest in $failedGtests.keys)
	{
		$message = '  {0}: {1}' -f $failedTest, $failedGtests[$failedTest]
		Write-Host $message
	}
    exit 1
}
else
{
	Write-Host "All tests passed!"
}