param([string]$Configuration = "Debug", [bool]$Verbose = $false)

Write-Host "Configuration: $Configuration"
Write-Host "Verbose: $Verbose"

$FAILED_TESTS = 0

if ($Verbose) {
	$VERBOSITY_PARAMETER="-v diag"
	$ENV:ADU_ENABLE_LOGGING="1"
}

dotnet test .\DiffGen\tests\UnitTests\UnitTests.csproj --logger "console;verbosity=detailed" --configuration $Configuration --no-build $VERBOSITY_PARAMETER
if ($LASTEXITCODE -ne 0) {
	$FAILED_TESTS = $FAILED_TESTS + 1
}
dotnet test .\DiffGen\tests\ArchiveUtilityTest\ArchiveUtilityTest.csproj --logger "console;verbosity=detailed" --configuration $Configuration --no-build $VERBOSITY_PARAMETER
if ($LASTEXITCODE -ne 0) {
	$FAILED_TESTS = $FAILED_TESTS + 1
}

dotnet test .\DiffGen\tests\EndToEndTests\EndToEndTests.csproj --logger "console;verbosity=detailed" --configuration $Configuration --no-build $VERBOSITY_PARAMETER
if ($LASTEXITCODE -ne 0) {
	$FAILED_TESTS = $FAILED_TESTS + 1
}

if ("$FAILED_TESTS" -eq "0") {
	Write-Host "All tests passed!"
	exit 0
}
else {
	Write-Host "${FAILED_TESTS} failed!"
	exit 1
}
