/**
 * @file recipe_type.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <string>

namespace diffs
{
enum class recipe_type : uint8_t
{
	copy               = 0,
	region             = 1,
	concatenation      = 2,
	bsdiff_delta       = 3,
	nested_diff        = 4,
	remainder_chunk    = 5,
	inline_asset       = 6,
	copy_source        = 7,
	zstd_delta         = 8,
	inline_asset_copy  = 9,
	zstd_compression   = 10,
	zstd_decompression = 11,
	all_zero           = 12,
	gz_decompression   = 13,
	last               = gz_decompression
};

bool is_valid_recipe_type(recipe_type value);
const std::string &get_recipe_type_string(recipe_type type);

} // namespace diffs