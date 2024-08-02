/**
 * @file cookbook.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include "item_definition.h"
#include "recipe_set.h"
#include "recipe_lookup.h"

namespace archive_diff::diffs::core
{
class cookbook
{
	public:
	void add_recipe(std::shared_ptr<recipe> &recipe);
	bool find_recipes_for_item(const item_definition &item, const recipe_set **recipes);
	const recipe_set_lookup get_all_recipes() const { return m_recipes; }

	private:
	recipe_lookup m_lookup;
	recipe_set_lookup m_recipes;
};
} // namespace archive_diff::diffs::core