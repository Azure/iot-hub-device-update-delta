/**
 * @file zlib_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zlib_decompression_recipe.h"

#include <io/compressed/zlib_decompression_reader.h>
#include <io/sequential/reader_factory.h>

#ifdef USE_TEMP_FILE_FOR_ZLIB_DECOMPRESSION_READER
	// This is a debug tool to validate the sequential reader implementation
    // is not causing issues in large payload scenarios that
    // are hard to fully debug.
	#include <io/file/temp_file.h>
	#include <io/sequential/basic_writer_wrapper.h>
	#include <io/compressed/zlib_decompression_writer.h>
#else
	#include <diffs/core/zlib_decompression_reader_factory.h>
#endif

namespace archive_diff::diffs::recipes::compressed
{
#ifdef USE_TEMP_FILE_FOR_ZLIB_DECOMPRESSION_READER
class zlib_decompression_reader_factory_using_temp_file : public io::reader_factory
{
	using decompression_reader = io::compressed::zlib_decompression_reader;
	using init_type            = io::compressed::zlib_helpers::init_type;
	using item_definition      = diffs::core::item_definition;
	using prepared_item        = diffs::core::prepared_item;

	public:
	zlib_decompression_reader_factory_using_temp_file(
		const item_definition &uncompressed,
		std::shared_ptr<prepared_item> &compressed_prepared_item,
		init_type init_type) :
		m_uncompressed_result(uncompressed), m_compressed_prepared_item(compressed_prepared_item),
		m_init_type(init_type)
	{}

	io::reader make_reader() override
	{
		auto compressed_reader = m_compressed_prepared_item->make_sequential_reader();

		auto temp_file  = std::make_shared<archive_diff::io::file::temp_file>();
		auto writer     = io::file::temp_file_writer::make_shared(temp_file);
		auto seq_writer = io::sequential::basic_writer_wrapper::make_shared(writer);

		io::compressed::zlib_decompression_writer decompression_writer{seq_writer, m_init_type};
		decompression_writer.write(*compressed_reader);

		auto uncompressed_reader = io::file::temp_file_io_device::make_reader(temp_file);

		return uncompressed_reader.slice(0, m_uncompressed_result.size());
	}

	private:
	item_definition m_uncompressed_result{};
	std::shared_ptr<prepared_item> m_compressed_prepared_item{};
	init_type m_init_type;
};
#endif

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

#ifdef USE_TEMP_FILE_FOR_ZLIB_DECOMPRESSION_READER
diffs::core::recipe::prepare_result zlib_decompression_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	auto compressed = items[0];

	using init_type                             = io::compressed::zlib_helpers::init_type;
	std::shared_ptr<io::reader_factory> factory = std::make_shared<zlib_decompression_reader_factory_using_temp_file>(
		m_result_item_definition, compressed, static_cast<init_type>(m_init_type));

	return std::make_shared<prepared_item>(
		m_result_item_definition, diffs::core::prepared_item::reader_kind{factory, {compressed}});
}
#else
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
#endif
} // namespace archive_diff::diffs::recipes::compressed