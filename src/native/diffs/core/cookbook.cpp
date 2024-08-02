/**
 * @file cookbook.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "cookbook.h"

namespace archive_diff::diffs::core
{
void cookbook::add_recipe(std::shared_ptr<recipe> &recipe)
{
	auto &item = recipe->get_result_item_definition();

	if (item.size() == 0)
	{
		return;
	}

	m_lookup.add(item, recipe);

	if (m_recipes.count(item) == 0)
	{
		m_recipes.insert(std::pair{item, recipe_set{}});
	}

	m_recipes.at(item).insert(recipe);
}

bool cookbook::find_recipes_for_item(const item_definition &item, const recipe_set **recipes)
{
	return m_lookup.find(item, recipes);
}
} // namespace archive_diff::diffs::core