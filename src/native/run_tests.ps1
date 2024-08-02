param([string] $BuildType, [string] $Configuration, [string] $Platform)

if ([string]::IsNullOrEmpty($BuildType)) {
    $BuildType = "msbuild"
}

if ([string]::IsNullOrEmpty($Configuration)) {
    Write-Host "Setting Configuration to Debug (default)"
    $Configuration = "Debug"
}

if ([string]::IsNullOrEmpty($Platform)) {
    $Platform = "x64"
}

function Usage {
    Write-Host "$PSCommandPath [msbuild|vs] [debug|release] [x64|x86]"
}


if (($BuildType.ToLower() -ne "msbuild") -and ($BuildType.ToLower() -ne "vs")) {
    Usage
    Exit 1
}

if (($Configuration.ToLower() -ne "debug") -and ($Configuration.ToLower() -ne "release")) {
    Usage
    Exit 1
}

if (($Platform.ToLower() -ne "x64") -and ($Platform.ToLower() -ne "x86")) {
    Usage
    Exit 1
}

Write-Host "Configuration: $Configuration"
Write-Host "Platform: $Platform"
Write-Host "BuildType: $BuildType"

if ($BuildType -eq "msbuild") {
    $cmakeBuildDir = & .\GetCMakeBuildDir $Platform-windows $Configuration
}
else {
    $cmakeBuildDir = Resolve-Path("./out/build/${Platform}-${Configuration}")
}

Write-Host "CMake Build Dir: $cmakeBuildDir"

function GetTestExePath {
    param([string] $TestExe)

    if ($BuildType -eq "msbuild") {
        return Resolve-Path "${cmakeBuildDir}/test/bin/$Configuration/${TestExe}"
    }

    return Resolve-Path "${cmakeBuildDir}/test/bin/${TestExe}"
}

$testData = Resolve-Path("$PSScriptRoot\..\..\data")

function RunTest {
    param([string] $TestExe, [bool] $UseTestData)

    $testExePath = GetTestExePath $TestExe

    if ($UseTestData) {
        Write-Host "Calling: $testExePath --test_data_root `"$testData`""
        & $testExePath --test_data_root "$testData"
    }
    else {
        Write-Host "Calling: $testExePath"
        & $testExePath
    }
}

RunTest io_gtest.exe
RunTest io_buffer_gtest.exe
RunTest io_hashed_gtest.exe
RunTest io_file_gtest.exe
RunTest io_compressed_gtest.exe $True
RunTest cpio_archives_gtest.exe

RunTest diffs_core_gtest.exe
RunTest diffs_recipes_basic_gtest.exe
RunTest diffs_recipes_compressed_gtest.exe $True
