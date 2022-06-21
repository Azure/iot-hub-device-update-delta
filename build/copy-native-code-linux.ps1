# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $repoRoot, [string] $vcpkgRoot, [string] $destination)

mkdir -Path $destination -Force > nul

$cmakeBuildDir="$repoRoot\src\out.x64-linux"

copy -Path $vcpkgRoot\packages\bsdiff_x64-linux\bin\bsdiff_diff -Destination $destination\bsdiff
copy -Path $vcpkgRoot\packages\bsdiff_x64-linux\bin\bsdiff_patch -Destination $destination\bspatch
copy -Path $vcpkgRoot\packages\bsdiff_x64-linux\share\bsdiff\copyright -Destination $destination\LICENSE.bsdiff
copy -Path $vcpkgRoot\packages\bzip2_x64-linux\share\bzip2\copyright -Destination $destination\LICENSE.bzip2

copy -Path $vcpkgRoot\packages\zlib_x64-windows\share\zlib\copyright -Destination $destination\LICENSE.zlib

copy -Path $vcpkgRoot\packages\zstd_x64-windows\share\zstd\copyright -Destination $destination\LICENSE.zstd

copy -Path $vcpkgRoot\packages\e2fsprogs_x64-linux\share\e2fsprogs\copyright -Destination $destination\LICENSE.e2fsprogs

copy -Path $repoRoot\src\tools\dumpextfs\dumpextfs -Destination $destination\dumpextfs

copy -Path $cmakeBuildDir\diffs\api\libadudiffapi.so -Destination $destination\libadudiffapi.so

copy -Path $cmakeBuildDir\tools\zstd_compress_file\zstd_compress_file -Destination $destination\zstd_compress_file

ls $destination