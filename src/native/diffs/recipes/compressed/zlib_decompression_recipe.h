/**
 * @file zlib_decompression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <optional>

#include <diffs/core/recipe_template.h>
#include <diffs/core/recipe_template_impl.h>

#include <diffs/core/recipe.h>

namespace archive_diff::diffs::recipes::compressed
{
using item_definition = diffs::core::item_definition;
using recipe          = diffs::core::recipe;
using kitchen         = diffs::core::kitchen;
using prepared_item   = diffs::core::prepared_item;

class zlib_decompression_recipe : public recipe
{
	public:
	zlib_decompression_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients);

	virtual std::string get_recipe_name() const override { return c_recipe_name; }

	virtual prepare_result prepare(kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const override;

	inline static const std::string c_recipe_name{"zlib_decompression"};
	using recipe_template = diffs::core::recipe_template_impl<zlib_decompression_recipe>;

	private:
	item_definition m_compressed_input{};
	uint64_t m_init_type;
};
} // namespace archive_diff::diffs::recipes::compressed