/**
 * @file recipe_template.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <memory>

#include "recipe.h"
#include "item_definition.h"

namespace archive_diff::diffs::core
{
class recipe_template
{
	public:
	virtual ~recipe_template() = default;

	virtual std::shared_ptr<recipe> create_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients) = 0;
};
} // namespace archive_diff::diffs::core