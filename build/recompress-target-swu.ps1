# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
param ([string] $repoRoot, [string] $zstd_compress_file_path)

# Script assumes that target.swu exists in repo already

$target = "$repoRoot/src/managed/DiffGen/tests/samples/diffs/swu/target.swu"
$target_wsl_path = & wsl wslpath $target.replace("\", "\\")

$target_recompressed = "$repoRoot/src/managed/DiffGen/tests/samples/diffs/swu/target-recompressed.swu"
$target_recompressed_wsl_path = & wsl wslpath $target_recompressed.replace("\", "\\")

$recompress_script="$repoRoot/src/scripts/recompress_swu/src/recompress_tool.py"
$recompress_script_wsl_path = & wsl wslpath $recompress_script.replace("\", "\\")

$zstd_compress_file_wsl_path = & wsl wslpath $zstd_compress_file_path.replace("\", "\\")

echo "wsl python3 $recompress_script_wsl_path $target_wsl_path $target_recompressed_wsl_path $zstd_compress_file_wsl_path"
& wsl python3 $recompress_script_wsl_path $target_wsl_path $target_recompressed_wsl_path $zstd_compress_file_wsl_path

echo "Created $target_recompressed"
ls $target_recompressed