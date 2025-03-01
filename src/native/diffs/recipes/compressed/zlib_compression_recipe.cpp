/**
 * @file zlib_compression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zlib_compression_recipe.h"

#include <io/compressed/zlib_compression_reader.h>
#include <io/sequential/reader_factory.h>

namespace archive_diff::diffs::recipes::compressed
{

zlib_compression_recipe::zlib_compression_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (number_ingredients.size() != 2)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"zlib_compression_recipe: Incorrect number count. Expected 2, found "
				+ std::to_string(number_ingredients.size()));
	}

	m_init_type         = static_cast<zlib_init_type>(number_ingredients[0]);
	m_compression_level = number_ingredients[1];

	if (item_ingredients.size() != 1)
	{
		throw std::exception();
	}

	m_uncompressed_input = item_ingredients[0];
}

class zlib_compression_reader_factory : public io::sequential::reader_factory
{
	public:
	zlib_compression_reader_factory(
		const item_definition &compressed,
		std::shared_ptr<prepared_item> &uncompressed,
		zlib_init_type init_type,
		uint64_t compression_level) : m_compressed(compressed), m_uncompresed(uncompressed), m_init_type(init_type)
	{
		if (compression_level == 0xFFFFFFFF)
		{
			m_compression_level = Z_DEFAULT_COMPRESSION;
		}
		else if (compression_level > 9)
		{
			std::string msg =
				"Invalid error message. Expected value from 0 to 9. Actual Value: " + std::to_string(compression_level);
			throw errors::user_exception(errors::error_code::recipe_zlib_compression_level_invalid, msg);
		}
		else
		{
			m_compression_level = static_cast<int>(compression_level);
		}
	}

	virtual std::unique_ptr<io::sequential::reader> make_sequential_reader() override
	{
		auto reader            = m_uncompresed->make_sequential_reader();
		auto uncompressed_size = reader->size();

		return std::make_unique<io::compressed::zlib_compression_reader>(
			std::move(reader), m_compression_level, uncompressed_size, m_compressed.size(), m_init_type);
	}

	private:
	item_definition m_compressed{};
	std::shared_ptr<prepared_item> m_uncompresed{};

	zlib_init_type m_init_type{};
	int m_compression_level{};
};

diffs::core::recipe::prepare_result zlib_compression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto uncompressed = items[0];

	std::shared_ptr<io::sequential::reader_factory> factory = std::make_shared<zlib_compression_reader_factory>(
		m_result_item_definition, uncompressed, m_init_type, m_compression_level);

	return std::make_shared<prepared_item>(
		m_result_item_definition,
		diffs::core::prepared_item::prepared_item::sequential_reader_kind{factory, {uncompressed}});
}
} // namespace archive_diff::diffs::recipes::compressed