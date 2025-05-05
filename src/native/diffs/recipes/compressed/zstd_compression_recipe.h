/**
 * @file zstd_compression_recipe.h
 * This recipe type is no longer supported - zstd compression output has changed.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <diffs/core/recipe.h>

#include <diffs/core/recipe_template.h>
#include <diffs/core/recipe_template_impl.h>

namespace archive_diff::diffs::recipes::compressed
{
using item_definition = diffs::core::item_definition;
using recipe          = diffs::core::recipe;
using kitchen         = diffs::core::kitchen;
using prepared_item   = diffs::core::prepared_item;

class zstd_compression_recipe : public recipe
{
	public:
	zstd_compression_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients) :
		recipe(result_item_definition, number_ingredients, item_ingredients)
	{
		throw errors::user_exception(
			errors::error_code::recipe_zstd_compression_not_supported,
			"zstd_compression_recipe::zstd_compression_recipe(): zstd_compression_recipe is no longer supported");
	}

	virtual std::string get_recipe_name() const override { return c_recipe_name; }

	inline static const std::string c_recipe_name{"zstd_compression"};
	using recipe_template = diffs::core::recipe_template_impl<zstd_compression_recipe>;

	protected:
	virtual prepare_result prepare(kitchen *, std::vector<std::shared_ptr<prepared_item>> &) const override
	{
		throw errors::user_exception(
			errors::error_code::recipe_zstd_compression_not_supported,
			"zstd_compression_recipe::prepare(): zstd_compression_recipe is no longer supported");
	}
};

} // namespace archive_diff::diffs::recipes::compressed
