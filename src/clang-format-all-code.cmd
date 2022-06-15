REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License.

REM This script assumes that clang-format is in the path
REM To add it to the path in command prompt use set PATH=%PATH%;<new location>
REM To add it in powershell use $env:Path+= ";<new location>"
REM The default location for clang in vs2022 is: "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\"
REM Since I use powershell currently, I added it to the path via:
REM $env:Path+= ";C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\"
REM If I used command prompt, I would do:
REM set PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\Llvm\x64\bin\

call :main
exit /b 0

:main
pushd diffs
call :run_clang_format *.cpp *.h
pushd diffs_test
call :run_clang_format *.cpp *.h
popd
pushd gtest
call :run_clang_format *.cpp *.h
popd
pushd api
call :run_clang_format *.cpp *.h
popd
popd
pushd io_utility
call :run_clang_format *.cpp *.h
pushd gtest
call :run_clang_format *.cpp *.h
popd
popd
pushd hash_utility
call :run_clang_format *.cpp *.h
popd
pushd error_utility
call :run_clang_format *.cpp *.h
popd
pushd tools
pushd applydiff
call :run_clang_format *.c
popd
pushd dumpdiff
call :run_clang_format *.cpp
popd
pushd dumpextfs
call :run_clang_format *.cpp *.h
popd
pushd zstd_compress_file
call :run_clang_format *.cpp *.h
popd
exit /b 0

:run_clang_format
echo Calling clang-format for %CD%
clang-format --style=file -i %*
exit /b 0