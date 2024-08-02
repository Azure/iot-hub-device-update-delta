/**
 * @file zstd_compression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_compression_recipe.h"

#include <io/compressed/zstd_compression_reader.h>
#include <io/sequential/reader_factory.h>

namespace archive_diff::diffs::recipes::compressed
{
zstd_compression_recipe::zstd_compression_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (item_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"zstd_compression_recipe: Incorrect item count. Expected 1, found "
				+ std::to_string(number_ingredients.size()));
	}

	m_uncompressed_input = item_ingredients[0];
}

class zstd_compression_reader_factory : public io::sequential::reader_factory
{
	public:
	zstd_compression_reader_factory(const item_definition &compressed, std::shared_ptr<prepared_item> &uncompressed) :
		m_compressed(compressed), m_uncompresed(uncompressed)
	{}

	virtual std::unique_ptr<io::sequential::reader> make_sequential_reader() override
	{
		auto reader            = m_uncompresed->make_sequential_reader();
		auto uncompressed_size = reader->size();

		return std::make_unique<io::compressed::zstd_compression_reader>(
			std::move(reader), 3, uncompressed_size, m_compressed.size());
	}

	private:
	item_definition m_compressed{};
	std::shared_ptr<prepared_item> m_uncompresed{};
};

diffs::core::recipe::prepare_result zstd_compression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto uncompressed = items[0];

	std::shared_ptr<io::sequential::reader_factory> factory =
		std::make_shared<zstd_compression_reader_factory>(m_result_item_definition, uncompressed);

	return std::make_shared<prepared_item>(
		m_result_item_definition,
		diffs::core::prepared_item::prepared_item::sequential_reader_kind{factory, {uncompressed}});
}
} // namespace archive_diff::diffs::recipes::compressed