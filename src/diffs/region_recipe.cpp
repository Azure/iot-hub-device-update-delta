/**
 * @file region_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "region_recipe.h"
#include "recipe_helpers.h"

const size_t RECIPE_PARAMETER_REGION_ITEM   = 0;
const size_t RECIPE_PARAMETER_REGION_OFFSET = 1;
const size_t RECIPE_PARAMETER_REGION_LENGTH = 2;

void diffs::region_recipe::apply(apply_context &context) const
{
	verify_parameter_count(3);

	auto archive_item = m_parameters[RECIPE_PARAMETER_REGION_ITEM].get_archive_item_value();

	auto &offset_param = m_parameters[RECIPE_PARAMETER_REGION_OFFSET];
	auto region_offset = convert_uint64_t<int>(
		offset_param.get_number_value(),
		error_utility::error_code::diff_region_offset_too_large,
		"Region offset too large");

	auto &length_param = m_parameters[RECIPE_PARAMETER_REGION_LENGTH];
	auto region_length = convert_uint64_t<int>(
		length_param.get_number_value(),
		error_utility::error_code::diff_region_length_too_large,
		"Region length too large");

	auto reader = archive_item->make_reader(context);
	io_utility::child_reader region_reader(reader.get(), region_offset, region_length);

	context.write_target(&region_reader);
}