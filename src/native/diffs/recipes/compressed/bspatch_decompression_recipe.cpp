/**
 * @file bspatch_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "bspatch_decompression_recipe.h"

#include <io/compressed/bspatch_decompression_reader.h>
#include <io/sequential/reader_factory.h>

#include <diffs/core/prepared_item.h>

namespace archive_diff::diffs::recipes::compressed
{
class bspatch_decompression_reader_factory : public io::sequential::reader_factory
{
	public:
	bspatch_decompression_reader_factory(
		const item_definition &uncompressed,
		std::shared_ptr<prepared_item> &compressed_prepared_item,
		std::shared_ptr<prepared_item> &dictionary_prepared_item) :
		m_uncompressed_result(uncompressed), m_compressed_prepared_item(compressed_prepared_item),
		m_dictionary_prepared_item(dictionary_prepared_item)
	{}

	virtual std::unique_ptr<io::sequential::reader> make_sequential_reader() override
	{
		auto compressed_reader = m_compressed_prepared_item->make_reader();
		auto dictionary_reader = m_dictionary_prepared_item->make_reader();

		return std::make_unique<io::compressed::bspatch_decompression_reader>(
			compressed_reader, m_uncompressed_result.size(), dictionary_reader);
	}

	private:
	item_definition m_uncompressed_result{};
	std::shared_ptr<prepared_item> m_compressed_prepared_item{};
	std::shared_ptr<prepared_item> m_dictionary_prepared_item{};
};

bspatch_decompression_recipe::bspatch_decompression_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (item_ingredients.size() != 2)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"bspatch_decompression_recipe: Incorrect item count. Expected 2, found "
				+ std::to_string(item_ingredients.size()));
	}

	m_compressed_input = item_ingredients[0];
	m_dictionary       = item_ingredients[1];
}

diffs::core::recipe::prepare_result bspatch_decompression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto compressed = items[0];
	auto dictionary = items[1];

	std::shared_ptr<io::sequential::reader_factory> factory =
		std::make_shared<bspatch_decompression_reader_factory>(m_result_item_definition, compressed, dictionary);

	return std::make_shared<prepared_item>(
		m_result_item_definition,
		diffs::core::prepared_item::sequential_reader_kind{factory, {compressed, dictionary}});
}
} // namespace archive_diff::diffs::recipes::compressed