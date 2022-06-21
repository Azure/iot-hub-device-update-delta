/**
 * @file gz_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "gz_decompression_recipe.h"

#include "archive_item.h"

#include "zlib_decompression_reader.h"

void diffs::gz_decompression_recipe::apply(apply_context &context) const
{
	std::string msg = "diffs::gz_decompression_recipe::apply(): Apply not supported.";
	throw error_utility::user_exception(
		error_utility::error_code::diff_recipe_gz_decompression_recipe_not_supported, msg);
}

std::unique_ptr<io_utility::reader> diffs::gz_decompression_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(1);

	auto item = m_parameters[0].get_archive_item_value();

	auto reader = std::make_unique<io_utility::zlib_decompression_reader>(
		std::move(item->make_reader(context)),
		m_blobdef.m_length,
		io_utility::zlib_decompression_reader::init_type::gz);

	auto temp_reader = std::make_unique<io_utility::zlib_decompression_reader>(
		std::move(item->make_reader(context)),
		m_blobdef.m_length,
		io_utility::zlib_decompression_reader::init_type::gz);

	return reader;
}