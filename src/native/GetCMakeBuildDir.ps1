param([string]$VcpkgTriplet, [string]$BuildType)
$cmakeBuildDir = "..\out\native\$VcpkgTriplet\$BuildType"
if (-not (Test-Path $cmakeBuildDir)) {
	mkdir $cmakeBuildDir | Out-Null
}
Write-Output (Resolve-Path($cmakeBuildDir)).Path

