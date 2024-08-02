/**
 * @file recipe_template_impl.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe_template.h"
#include "item_definition.h"

namespace archive_diff::diffs::core
{
template <class RecipeT>
class recipe_template_impl : public recipe_template
{
	public:
	recipe_template_impl() = default;

	virtual std::shared_ptr<recipe> create_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients)
	{
		return std::make_shared<RecipeT>(result_item_definition, number_ingredients, item_ingredients);
	}
};
} // namespace archive_diff::diffs::core
