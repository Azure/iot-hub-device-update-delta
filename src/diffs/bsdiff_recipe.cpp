/**
 * @file bsdiff_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "bsdiff_recipe.h"

#include "recipe_helpers.h"

#include "bsdiff.h"

#include "bspatch_reader.h"

io_utility::unique_reader call_bspatch(
	diffs::apply_context &context,
	const diffs::archive_item *source,
	const diffs::archive_item *delta,
	uint64_t target_length)
{
	auto source_reader = source->make_reader(context);
	auto delta_reader  = delta->make_reader(context);

	auto target_reader = std::make_unique<io_utility::bspatch_decompression_reader>(
		std::move(source_reader), std::move(delta_reader), target_length);

	return target_reader;
}

void diffs::bsdiff_recipe::apply(apply_context &context) const
{
	const auto &source_param = m_parameters[RECIPE_PARAMETER_SOURCE];
	const auto &delta_param  = m_parameters[RECIPE_PARAMETER_DELTA];

	auto source = source_param.get_archive_item_value();
	auto delta  = delta_param.get_archive_item_value();

	auto reader = call_bspatch(context, source, delta, context.get_target_length());

	context.write_target(reader.get());
}
