/**
 * @file zlib_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zlib_decompression_recipe.h"

#include <io/compressed/zlib_decompression_reader.h>
#include <io/sequential/reader_factory.h>

#include <diffs/core/zlib_decompression_reader_factory.h>

namespace archive_diff::diffs::recipes::compressed
{
zlib_decompression_recipe::zlib_decompression_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (number_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"zlib_decompression_recipe: Incorrect number count. Expected 1, found "
				+ std::to_string(number_ingredients.size()));
	}

	if (item_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"zlib_decompression_recipe: Incorrect item count. Expected 1, found "
				+ std::to_string(item_ingredients.size()));
	}

	m_init_type        = number_ingredients[0];
	m_compressed_input = item_ingredients[0];
}

diffs::core::recipe::prepare_result zlib_decompression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto compressed = items[0];

	using init_type                                         = io::compressed::zlib_helpers::init_type;
	std::shared_ptr<io::sequential::reader_factory> factory = std::make_shared<core::zlib_decompression_reader_factory>(
		m_result_item_definition, compressed, static_cast<init_type>(m_init_type));

	return std::make_shared<prepared_item>(
		m_result_item_definition, diffs::core::prepared_item::sequential_reader_kind{factory, {compressed}});
}
} // namespace archive_diff::diffs::recipes::compressed