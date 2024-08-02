param([string]$Configuration)

if ([string]::IsNullOrEmpty($Configuration)) {
	$Configuration = "Debug"
}

& dotnet build .\DiffGen\diff-generation.sln --configuration $Configuration