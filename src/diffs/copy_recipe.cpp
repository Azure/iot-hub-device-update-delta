/**
 * @file copy_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "copy_recipe.h"
#include "archive_item.h"

static bool is_basic_chunk(const diffs::archive_item &item)
{
	return (item.get_type() == diffs::archive_item_type::chunk) && !item.has_recipe();
}

static void apply_basic_chunk(const diffs::archive_item &chunk, diffs::apply_context &context)
{
	uint64_t offset = chunk.get_offset();
	uint64_t length = chunk.get_length();

	uint64_t remaining   = length;
	uint64_t read_offset = 0;

	context.flush_target();

	auto target_reader = context.get_target_reader(offset, length);
	context.write_target(target_reader.get());
}

void diffs::copy_recipe::apply(apply_context &context) const
{
	verify_parameter_count(1);

	auto archive_item = m_parameters[0].get_archive_item_value();

	if (is_basic_chunk(*archive_item))
	{
		apply_basic_chunk(*archive_item, context);
		return;
	}

	archive_item->apply(context);
}

io_utility::unique_reader diffs::copy_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(1);

	auto archive_item = m_parameters[0].get_archive_item_value();

	if ((archive_item->get_type() == diffs::archive_item_type::chunk) && !archive_item->has_recipe())
	{
		uint64_t offset = archive_item->get_offset();
		uint64_t length = archive_item->get_length();

		context.flush_target();

		return context.get_target_reader(offset, length);
	}

	return archive_item->make_reader(context);
}
