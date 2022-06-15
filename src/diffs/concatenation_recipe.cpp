/**
 * @file concatenation_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "concatenation_recipe.h"
#include "archive_item.h"
#include "chained_reader.h"

void diffs::concatenation_recipe::apply(apply_context &context) const
{
	for (const auto &param : m_parameters)
	{
		auto item = param.get_archive_item_value();
		item->apply(context);
	}
}

io_utility::unique_reader diffs::concatenation_recipe::make_reader(apply_context &context) const
{
	std::vector<io_utility::unique_reader> readers;

	for (const auto &param : m_parameters)
	{
		auto item = param.get_archive_item_value();
		readers.emplace_back(item->make_reader(context));
	}

	return std::make_unique<io_utility::chained_reader>(std::move(readers));
}
