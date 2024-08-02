# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# This script assumes that clang-format is in the path
# To add it to the path in command prompt use set PATH=%PATH%;<new location>
# To add it in powershell use $env:Path+= ";<new location>"
# The default location for clang in vs2022 is: "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\"
# Since I use powershell currently, I added it to the path via:
# $env:Path+= ";C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\"
# If I used command prompt, I would do:
# set PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\

$clangFormatDir=""

function ClangFormatAllDirs([string]$folder)
{
	$all_code_dirs = Get-ChildItem -Path $folder -Recurse -Directory -Force -ErrorAction SilentlyContinue | Select-Object FullName

	foreach ($dir in $all_code_dirs)
	{
		$cpp_file_count = CppAllFilesCount $dir.FullName
		$dir_full_name = $dir.FullName
		if ($cpp_file_count -eq 0)
		{
			continue
		}

		echo "Found $cpp_file_count cpp files in $dir_full_name"

		RunClangFormat $dir.FullName
	}
}

function RunClangFormat([string]$folder)
{
	$files = Get-ChildItem $folder
	| Where-Object Extension -in ('.h', '.cpp')
	| Select-Object FullName;

	foreach ($file in $files)
	{
		$fileName = $file.FullName
		echo "${clangFormatDir}clang-format --style=file -i $fileName"
		&${clangFormatDir}clang-format --style=file -i $fileName
	}
}

function CppHeaderFilesCount([string]$folder)
{
	(Get-ChildItem -Path $folder -force | Where-Object Extension -in ('.h') | Measure-Object).Count
}

function CppCodeFilesCount([string]$folder)
{
	(Get-ChildItem -Path $folder -force | Where-Object Extension -in ('.cpp') | Measure-Object).Count
}

function CppAllFilesCount([string]$folder)
{
	(Get-ChildItem -Path $folder -force | Where-Object Extension -in ('.h','.cpp') | Measure-Object).Count
}

if (( ${env:path}.Split(";") | Where { Test-Path -Path "$_\clang-format.exe" } | Measure ).Count -eq 0)
{
	echo "Couldn't find clang-format.exe in path."

	$possiblePaths = (Get-ChildItem -Path $env:ProgramFiles -Recurse -Filter 'clang-format.exe' -ErrorAction SilentlyContinue | Where {$_.FullName -like '*x64\bin*'})
	$entryCounts = ($possiblePaths | Measure).Count

	if ($entryCounts -eq 0)
	{
		echo "Couldn't find a path for clang-format in program files."
		exit 1
	}

	echo "Will try to use $clangFormatDir for clang-format.exe"
	$clangFormatDir = $possiblePaths[0].Directory.FullName
	if (!$clangFormatDir.EndsWith('\\') || !clangFormatDir.EndsWith('/'))
	{
		$clangFormatDir = $clangFormatDir + '/'
	}
}

clangformatalldirs "hashing"
clangformatalldirs "error"
clangformatalldirs "io"
clangformatalldirs "test_utility"
clangformatalldirs "tools"
clangformatalldirs "diffs"
clangformatalldirs "archives"
