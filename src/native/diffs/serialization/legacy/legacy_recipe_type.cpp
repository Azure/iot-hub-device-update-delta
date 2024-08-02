/**
 * @file legacy_recipe_type.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "legacy_recipe_type.h"

#include <map>

namespace archive_diff::diffs::serialization::legacy
{
static std::map<std::string, legacy_recipe_type> g_string_to_legacy_recipe_type_map = {
	{"copy_recipe", legacy_recipe_type::copy},
	{"region_recipe", legacy_recipe_type::region},
	{"concatenation_recipe", legacy_recipe_type::concatentation},
	{"bsdiff_recipe", legacy_recipe_type::bsdiff},
	{"nested_recipe", legacy_recipe_type::nested},
	{"remainder_recipe", legacy_recipe_type::remainder},
	{"inline_asset_recipe", legacy_recipe_type::inline_asset},
	{"copy_source_recipe", legacy_recipe_type::copy_source},
	{"zstd_delta_recipe", legacy_recipe_type::zstd_delta},
	{"inline_asset_copy_recipe", legacy_recipe_type::inline_asset_copy},
	{"zstd_compression_recipe", legacy_recipe_type::zstd_compression},
	{"zstd_decompression_recipe", legacy_recipe_type::zstd_decompression},
	{"all_zero_recipe", legacy_recipe_type::all_zero},
	{"gz_decompression_recipe", legacy_recipe_type::gz_decompression}};

legacy_recipe_type legacy_recipe_type_from_string(const std::string &recipe_name)
{
	if (!g_string_to_legacy_recipe_type_map.count(recipe_name))
	{
		throw std::exception();
	}

	return g_string_to_legacy_recipe_type_map[recipe_name];
}
} // namespace archive_diff::diffs::serialization::legacy