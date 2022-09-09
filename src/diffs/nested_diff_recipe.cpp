/**
 * @file nested_diff_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "nested_diff_recipe.h"

#include "recipe_helpers.h"
#include "diff.h"
#include "diff_reader.h"
#include "diff_writer_context.h"
#include "diff_reader_context.h"

void diffs::nested_diff_recipe::write(diff_writer_context &context)
{
	uint8_t parameter_count = 2u;
	context.write(parameter_count);

	m_parameters[RECIPE_PARAMETER_DELTA].write(context);
	m_parameters[RECIPE_PARAMETER_SOURCE].write(context);
}

void diffs::nested_diff_recipe::read(diff_reader_context &context)
{
	read_parameters(context);

	auto offset = context.m_chunk_table_total;

	recipe_parameter offset_parameter{offset};
	add_parameter(std::move(offset_parameter));
}

void diffs::nested_diff_recipe::apply(apply_context &context) const
{
	verify_parameter_count(3);

	const auto &delta_param  = m_parameters[RECIPE_PARAMETER_DELTA];
	const auto &source_param = m_parameters[RECIPE_PARAMETER_SOURCE];
	const auto &offset_param = m_parameters[RECIPE_PARAMETER_NESTED_OFFSET];

	const auto source = source_param.get_archive_item_value();
	const auto delta  = delta_param.get_archive_item_value();

	auto diff_reader = delta->make_reader(context);
	diffs::diff diff(diff_reader.get());

	auto basis_reader         = source->make_reader(context);
	auto inline_assets_reader = diff.make_inline_assets_reader(diff_reader.get());
	auto remainder_reader     = diff.make_remainder_reader(diff_reader.get());

	diffs::apply_context nested_context = diffs::apply_context::nested_context(
		&context,
		std::move(basis_reader),
		std::move(inline_assets_reader),
		std::move(remainder_reader),
		diff.get_target_size());

	diff.apply(nested_context);
}

#if 0
// need to block copy recipes within compressed ZSTD targets, because there is no
// target_reader in this context
io_utility::unique_reader diffs::nested_diff_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(3);

	const auto &source_param = m_parameters[RECIPE_PARAMETER_SOURCE];
	const auto &delta_param  = m_parameters[RECIPE_PARAMETER_DELTA];

	const auto source = source_param.get_archive_item_value();
	const auto delta  = delta_param.get_archive_item_value();

	auto source_reader = source->make_reader(context);
	auto delta_reader  = delta->make_reader(context);

	return std::make_unique<diff_reader>(context, std::move(source_reader), std::move(delta_reader));
}
#else
io_utility::unique_reader diffs::nested_diff_recipe::make_reader(apply_context &context) const
{
	return recipe::make_reader(context);
}
#endif