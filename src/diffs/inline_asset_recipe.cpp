/**
 * @file inline_asset_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "inline_asset_recipe.h"

#include "recipe_helpers.h"

void diffs::inline_asset_recipe::apply(apply_context &context) const
{
	auto reader = make_reader(context);
	context.write_target(reader.get());
}

io_utility::unique_reader diffs::inline_asset_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(2);

	uint64_t inline_asset_offset = m_parameters[RECIPE_PARAMETER_OFFSET].get_number_value();
	if (inline_asset_offset > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_inline_asset_offset_too_large,
			"Inline asset offset too large",
			inline_asset_offset);
	}

	uint64_t inline_asset_length = m_parameters[RECIPE_PARAMETER_LENGTH].get_number_value();
	if (inline_asset_length > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_inline_asset_length_too_large,
			"Inline asset length too large",
			inline_asset_offset);
	}

	return context.get_inline_asset_reader(inline_asset_offset, inline_asset_length);
}