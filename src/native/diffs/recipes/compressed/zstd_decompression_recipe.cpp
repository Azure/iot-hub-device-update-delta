/**
 * @file zstd_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_decompression_recipe.h"

#include <io/compressed/zstd_decompression_reader.h>
#include <io/sequential/reader_factory.h>

namespace archive_diff::diffs::recipes::compressed
{
class zstd_decompression_reader_factory : public io::sequential::reader_factory
{
	public:
	zstd_decompression_reader_factory(
		const item_definition &uncompressed, std::shared_ptr<prepared_item> &compressed_prepared_item) :
		m_uncompressed_result(uncompressed), m_compressed_prepared_item(compressed_prepared_item)
	{}

	zstd_decompression_reader_factory(
		const item_definition &uncompressed,
		std::shared_ptr<prepared_item> &compressed_prepared_item,
		std::shared_ptr<prepared_item> &dictionary_prepared_item) :
		m_uncompressed_result(uncompressed), m_compressed_prepared_item(compressed_prepared_item),
		m_dictionary_prepared_item(dictionary_prepared_item)
	{}

	virtual std::unique_ptr<io::sequential::reader> make_sequential_reader() override
	{
		auto compressed_reader = m_compressed_prepared_item->make_sequential_reader();

		if (m_dictionary_prepared_item.get() == nullptr)
		{
			return std::make_unique<io::compressed::zstd_decompression_reader>(
				std::move(compressed_reader), m_uncompressed_result.size());
		}

		auto dictionary_reader = m_dictionary_prepared_item->make_sequential_reader();

		return std::make_unique<io::compressed::zstd_decompression_reader>(
			std::move(compressed_reader), m_uncompressed_result.size(), *dictionary_reader.get());
	}

	private:
	item_definition m_uncompressed_result{};
	std::shared_ptr<prepared_item> m_compressed_prepared_item{};
	std::shared_ptr<prepared_item> m_dictionary_prepared_item{};
};

zstd_decompression_recipe::zstd_decompression_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (item_ingredients.size() == 2)
	{
		m_dictionary = item_ingredients[1];
	}
	else if (item_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"zstd_decompression_recipe: Incorrect item count. Expected 1 or 2, found "
				+ std::to_string(number_ingredients.size()));
	}

	m_compressed_input = item_ingredients[0];
}

diffs::core::recipe::prepare_result zstd_decompression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto compressed = items[0];

	std::shared_ptr<io::sequential::reader_factory> factory;

	if (items.size() == 2)
	{
		auto dictionary = items[1];
		factory = std::make_shared<zstd_decompression_reader_factory>(m_result_item_definition, compressed, dictionary);
	}
	else
	{
		factory = std::make_shared<zstd_decompression_reader_factory>(m_result_item_definition, compressed);
	}

	return std::make_shared<prepared_item>(
		m_result_item_definition, diffs::core::prepared_item::sequential_reader_kind{factory, items});
}
} // namespace archive_diff::diffs::recipes::compressed