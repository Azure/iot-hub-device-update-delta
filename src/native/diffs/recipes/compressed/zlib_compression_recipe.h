/**
 * @file zlib_compression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <diffs/core/recipe.h>

#include <diffs/core/recipe_template.h>
#include <diffs/core/recipe_template_impl.h>

#include <io/compressed/zlib_helpers.h>

namespace archive_diff::diffs::recipes::compressed
{
using item_definition = diffs::core::item_definition;
using recipe          = diffs::core::recipe;
using kitchen         = diffs::core::kitchen;
using prepared_item   = diffs::core::prepared_item;

using zlib_init_type = io::compressed::zlib_helpers::init_type;

class zlib_compression_recipe : public recipe
{
	public:
	zlib_compression_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients);

	virtual std::string get_recipe_name() const override { return c_recipe_name; }

	inline static const std::string c_recipe_name{"zlib_compression"};
	using recipe_template = diffs::core::recipe_template_impl<zlib_compression_recipe>;

	protected:
	virtual prepare_result prepare(kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const override;

	private:
	zlib_init_type m_init_type{};
	uint64_t m_compression_level{};

	item_definition m_uncompressed_input{};
};

} // namespace archive_diff::diffs::recipes::compressed
