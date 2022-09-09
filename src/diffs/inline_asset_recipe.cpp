/**
 * @file inline_asset_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "inline_asset_recipe.h"

#include "recipe_helpers.h"
#include "diff_writer_context.h"
#include "diff_reader_context.h"

void diffs::inline_asset_recipe::write(diff_writer_context &context)
{
	uint8_t parameter_count = 0;
	context.write(parameter_count);
}

void diffs::inline_asset_recipe::read(diff_reader_context &context)
{
	read_parameters(context);

	auto offset = context.m_inline_asset_total;

	recipe_parameter offset_parameter{offset};
	add_parameter(std::move(offset_parameter));

	context.m_inline_asset_total += context.m_current_item_blobdef.m_length;

	recipe_parameter length_parameter{context.m_current_item_blobdef.m_length};
	add_parameter(std::move(length_parameter));
}

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

uint64_t diffs::inline_asset_recipe::get_inline_asset_byte_count() const
{
	uint64_t inline_asset_length = m_blobdef.m_length;
	return inline_asset_length;
}