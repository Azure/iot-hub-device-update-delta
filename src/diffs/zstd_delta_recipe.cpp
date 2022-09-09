/**
 * @file zstd_delta_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_delta_recipe.h"
#include "archive_item.h"

#include "zstd_decompression_reader.h"

const size_t RECIPE_PARAMETER_DELTA  = 0;
const size_t RECIPE_PARAMETER_SOURCE = 1;

void diffs::zstd_delta_recipe::apply(apply_context &context) const
{
	auto reader = make_reader(context);
	context.write_target(reader.get());
}

std::unique_ptr<io_utility::reader> diffs::zstd_delta_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(2);

	auto basis_item   = m_parameters[RECIPE_PARAMETER_SOURCE].get_archive_item_value();
	auto basis_reader = basis_item->make_reader(context);

	auto delta_item           = m_parameters[RECIPE_PARAMETER_DELTA].get_archive_item_value();
	auto decompression_reader = std::make_unique<io_utility::zstd_decompression_reader>(
		std::move(delta_item->make_reader(context)), m_blobdef.m_length, basis_reader.get());

	return decompression_reader;
}