/**
 * @file recipe_names.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

namespace diffs
{
static const char all_zero_recipe_name[]           = "all_zero_recipe";
static const char bsdiff_recipe_name[]             = "bsdiff_recipe";
static const char concatenation_recipe_name[]      = "concatenation_recipe";
static const char copy_recipe_name[]               = "copy_recipe";
static const char copy_source_recipe_name[]        = "copy_source_recipe";
static const char gz_decompression_recipe_name[]   = "gz_decompression_recipe";
static const char inline_asset_recipe_name[]       = "inline_asset_recipe";
static const char inline_asset_copy_recipe_name[]  = "inline_asset_copy_recipe";
static const char nested_diff_recipe_name[]        = "nested_diff_recipe";
static const char region_recipe_name[]             = "region_recipe";
static const char remainder_chunk_recipe_name[]    = "remainder_chunk_recipe";
static const char zstd_compression_recipe_name[]   = "zstd_compression_recipe";
static const char zstd_decompression_recipe_name[] = "zstd_decompression_recipe";
static const char zstd_delta_recipe_name[]         = "zstd_delta_recipe";
} // namespace diffs