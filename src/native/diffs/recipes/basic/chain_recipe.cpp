/**
 * @file chain_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "chain_recipe.h"

namespace archive_diff::diffs::recipes::basic
{
chain_recipe::chain_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{}

diffs::core::recipe::prepare_result chain_recipe::prepare(
	kitchen *, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	diffs::core::prepared_item::chain_kind chain;

	uint64_t total_length{};

	if (m_item_ingredients.size() != items.size())
	{
		throw errors::user_exception(
			errors::error_code::recipe_chain_item_and_recipe_mismatch,
			"Mismatch between count of items/ingredients for chain recipe and prepared item.");
	}

	for (size_t i = 0; i < items.size(); i++)
	{
		auto &item       = items[i]->get_item_definition();
		auto &ingredient = m_item_ingredients[i];

		if (item != ingredient)
		{
			throw errors::user_exception(
				errors::error_code::recipe_chain_item_and_recipe_mismatch,
				"Mismatch between items/ingredients for chain recipe and prepared item.");
		}
		total_length += item.size();
	}

	chain.m_items = items;

	if (total_length != m_result_item_definition.size())
	{
		throw errors::user_exception(
			errors::error_code::recipe_chain_total_item_length_mismatch,
			"Total items length for chain is " + std::to_string(total_length) + ", but result length is "
				+ std::to_string(m_result_item_definition.size()));
	}

	return std::make_shared<prepared_item>(m_result_item_definition, chain);
}
} // namespace archive_diff::diffs::recipes::basic