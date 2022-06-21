REM Copyright (c) Microsoft Corporation.
REM Licensed under the MIT License.

SETLOCAL

@ECHO OFF

SET VCPKG_ROOT=%1
SET PORT_ROOT=%2
SET TRIPLET=%3

if [%PORT_ROOT%]==[] goto :Usage

if [%TRIPLET%]==[] SET TRIPLET=x64-windows

echo Configuring VCPKG at %VCPKG_ROOT% using ports at %PORT_ROOT% for triplet %TRIPLET%

@ECHO ON

git clone https://github.com/microsoft/vcpkg %VCPKG_ROOT%

CALL %VCPKG_ROOT%\bootstrap-vcpkg.bat

CALL %VCPKG_ROOT%\vcpkg.exe install zlib:%TRIPLET% --overlay-ports=%PORT_ROOT%
CALL %VCPKG_ROOT%\vcpkg.exe install zstd:%TRIPLET% --overlay-ports=%PORT_ROOT%
CALL %VCPKG_ROOT%\vcpkg.exe install ms-gsl:%TRIPLET% --overlay-ports=%PORT_ROOT%
CALL %VCPKG_ROOT%\vcpkg.exe install gtest:%TRIPLET% --overlay-ports=%PORT_ROOT%
CALL %VCPKG_ROOT%\vcpkg.exe install bzip2:%TRIPLET% --overlay-ports=%PORT_ROOT%
CALL %VCPKG_ROOT%\vcpkg.exe install bsdiff:%TRIPLET% --overlay-ports=%PORT_ROOT%

CALL %VCPKG_ROOT%\vcpkg.exe integrate install

exit /b 0

:Usage
echo Usage:
echo    %0 ^<vcpkg root^> ^<port root^> [^<triplet^>]
echo    ^<vcpkg root^> is the intall target for vcpkg repo
echo    ^<port root^> is the location of vcpkg ports - in the ADU repo this is under vcpkg\ports
echo    [^<triplet^>] is optional and can be omitted, default is x64-windows
exit /b 1