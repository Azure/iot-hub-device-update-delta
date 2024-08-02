/**
 * @file legacy_recipe_type.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <stdint.h>
#include <string>

namespace archive_diff::diffs::serialization::legacy
{
enum class legacy_recipe_type : uint32_t
{
	copy               = 0,
	region             = 1,
	concatentation     = 2,
	bsdiff             = 3,
	nested             = 4,
	remainder          = 5,
	inline_asset       = 6,
	copy_source        = 7,
	zstd_delta         = 8,
	inline_asset_copy  = 9,
	zstd_compression   = 10,
	zstd_decompression = 11,
	all_zero           = 12,
	gz_decompression   = 13,
};

legacy_recipe_type legacy_recipe_type_from_string(const std::string &recipe_name);
} // namespace archive_diff::diffs::serialization::legacy