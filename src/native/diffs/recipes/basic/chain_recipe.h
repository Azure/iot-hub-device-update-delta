/**
 * @file chain_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
/**
 * @file chain_recipe.h
 *
 * Licensed under the MIT License.
 */
#pragma once

#include <io/reader.h>

#include <diffs/core/recipe.h>
#include <diffs/core/recipe_template.h>
#include <diffs/core/recipe_template_impl.h>

namespace archive_diff::diffs::recipes::basic
{
using item_definition = diffs::core::item_definition;
using recipe          = diffs::core::recipe;
using kitchen         = diffs::core::kitchen;
using prepared_item   = diffs::core::prepared_item;

class chain_recipe : public recipe
{
	public:
	chain_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients);
	virtual std::string get_recipe_name() const override { return c_recipe_name; }
	virtual prepare_result prepare(kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const override;

	inline static const std::string c_recipe_name = "chain";
	using recipe_template                         = diffs::core::recipe_template_impl<chain_recipe>;
};
} // namespace archive_diff::diffs::recipes::basic
