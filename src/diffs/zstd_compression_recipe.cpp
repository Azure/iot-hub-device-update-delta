/**
 * @file zstd_compression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_compression_recipe.h"

#include "recipe_helpers.h"

const size_t RECIPE_PARAMETER_ZSTD_COMPRESS_ITEM          = 0;
const size_t RECIPE_PARAMETER_ZSTD_COMPRESS_MAJOR_VERSION = 1;
const size_t RECIPE_PARAMETER_ZSTD_COMPRESS_MINOR_VERSION = 2;
const size_t RECIPE_PARAMETER_ZSTD_COMPRESS_LEVEL         = 3;

void diffs::zstd_compression_recipe::apply(apply_context &context) const
{
	verify_parameter_count(4);

	auto archive_item = m_parameters[RECIPE_PARAMETER_ZSTD_COMPRESS_ITEM].get_archive_item_value();

	auto &major_version_param     = m_parameters[RECIPE_PARAMETER_ZSTD_COMPRESS_MAJOR_VERSION];
	auto &minor_version_param     = m_parameters[RECIPE_PARAMETER_ZSTD_COMPRESS_MINOR_VERSION];
	auto &compression_level_param = m_parameters[RECIPE_PARAMETER_ZSTD_COMPRESS_LEVEL];

	auto major_version = convert_uint64_t<int>(
		major_version_param.get_number_value(),
		error_utility::error_code::diff_zstd_number_parameter_too_large,
		"Major version too large");
	auto minor_version = convert_uint64_t<int>(
		minor_version_param.get_number_value(),
		error_utility::error_code::diff_zstd_number_parameter_too_large,
		"Minor version too large");
	auto compression_level = convert_uint64_t<int>(
		compression_level_param.get_number_value(),
		error_utility::error_code::diff_zstd_number_parameter_too_large,
		"Compression level too large");

	auto compressed_context = apply_context::zstd_compressed_context(
		&context, major_version, minor_version, compression_level, archive_item->get_length());
	archive_item->apply(compressed_context);
}