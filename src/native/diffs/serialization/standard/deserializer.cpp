/**
 * @file deserializer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "deserializer.h"
#include "constants.h"

#include <io/reader.h>
#include <io/basic_reader_factory.h>
#include <io/sequential/basic_reader_wrapper.h>
#include <io/file/io_device.h>
#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>
#include <io/compressed/zlib_compression_writer.h>

#include <errors/user_exception.h>
#include <hashing/hash.h>

#include <diffs/core/recipe_template.h>
#include <diffs/core/prepared_item.h>
#include <diffs/core/item_definition_helpers.h>

#include <diffs/recipes/compressed/zlib_decompression_recipe.h>

#include <diffs/core/zlib_decompression_reader_factory.h>

namespace archive_diff::diffs::serialization::standard
{
bool deserializer::is_this_format(io::reader &reader, std::string *reason)
{
	const auto c_header_size = (g_DIFF_MAGIC_VALUE.size() + sizeof(g_STANDARD_DIFF_VERSION));
	if (reader.size() < c_header_size)
	{
		*reason = "Too small. Found: " + std::to_string(reader.size())
		        + ", Header Size Required: " + std::to_string(c_header_size);
		return false;
	}

	char magic[4]{};
	reader.read(0, std::span<char>{magic, sizeof(magic)});

	if (0 != std::memcmp(magic, g_DIFF_MAGIC_VALUE.data(), sizeof(magic)))
	{
		std::string magic_str(magic, 4);
		*reason = "Wrong magic found. Expected: " + g_DIFF_MAGIC_VALUE + ", Found: " + magic_str;
		return false;
	}

	uint64_t version;

	reader.read(4, &version);

	if (version != g_STANDARD_DIFF_VERSION)
	{
		*reason = "Wrong version. Expected: " + std::to_string(g_STANDARD_DIFF_VERSION)
		        + ", Found: " + std::to_string(version);
		return false;
	}

	return true;
}

void deserializer::read(io::reader &reader)
{
	io::sequential::basic_reader_wrapper seq(reader);
	read_header(seq);
	read_supported_recipe_types(seq);
	read_recipes(seq);
	read_inline_assets(seq, reader);
	read_remainder(seq, reader);

	auto nested_archives_offet  = seq.tellg();
	auto remaining_length       = reader.size() - nested_archives_offet;
	auto nested_archives_reader = reader.slice(nested_archives_offet, remaining_length);

	read_nested_archives(nested_archives_reader);
}

void deserializer::read_header(io::sequential::reader &seq)
{
	char magic[4]{};

	seq.read(std::span<char>{magic, sizeof(magic)});

	if (0 != std::memcmp(magic, g_DIFF_MAGIC_VALUE.data(), sizeof(magic)))
	{
		std::string magic_string(magic, 4);
		std::string msg = "Not a valid diff file - invalid magic. Found: " + magic_string;
		throw errors::user_exception(errors::error_code::diff_magic_header_wrong, msg);
	}

	uint64_t version;

	seq.read(&version);

	if (version != g_STANDARD_DIFF_VERSION)
	{
		std::string msg = "Not a valid version. Expected: " + std::to_string(g_STANDARD_DIFF_VERSION)
		                + ", Found: " + std::to_string(version);
		throw errors::user_exception(errors::error_code::diff_version_wrong, msg);
	}

	auto target_item = core::item_definition::read(seq, core::item_definition::standard);
	m_archive->set_archive_item(target_item);

	auto source_item = core::item_definition::read(seq, core::item_definition::standard);
	m_archive->set_source_item(source_item);
}

void deserializer::add_recipe(
	const std::string &recipe_name,
	const core::item_definition &result_item,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<core::item_definition> &item_ingredients)
{
	auto id = m_archive->add_supported_recipe_type(recipe_name);

	auto recipe = m_archive->create_recipe(id, result_item, number_ingredients, item_ingredients);

	add_recipe(recipe);
}

void deserializer::read_supported_recipe_types(io::sequential::reader &seq)
{
	uint32_t supported_recipe_type_count;
	seq.read(&supported_recipe_type_count);

	for (uint32_t i = 0; i < supported_recipe_type_count; i++)
	{
		std::string recipe_type_name;
		seq.read(&recipe_type_name);
	}
}

void deserializer::read_recipe(io::sequential::reader &seq, const core::item_definition &result_item)
{
	uint32_t recipe_type_id;
	seq.read(&recipe_type_id);

	uint64_t numbers_count;
	seq.read(&numbers_count);

	std::vector<uint64_t> number_ingredients;
	while (numbers_count)
	{
		uint64_t number;
		seq.read(&number);
		number_ingredients.push_back(number);
		numbers_count--;
	}

	uint64_t items_count;
	seq.read(&items_count);

	std::vector<core::item_definition> item_ingredients;
	while (items_count)
	{
		auto item = core::item_definition::read(seq, core::item_definition::serialization_options::standard);
		item_ingredients.push_back(item);
		items_count--;
	}

	auto recipe = m_archive->create_recipe(recipe_type_id, result_item, number_ingredients, item_ingredients);

	m_archive->add_recipe(recipe);
}

void deserializer::read_recipe_set(io::sequential::reader &seq)
{
	uint64_t recipes_count;
	seq.read(&recipes_count);

	auto result_item = core::item_definition::read(seq, diffs::core::item_definition::standard);

	for (uint64_t i = 0; i < recipes_count; i++)
	{
		read_recipe(seq, result_item);
	}
}

void deserializer::read_recipes(io::sequential::reader &seq)
{
	uint64_t result_count;
	seq.read(&result_count);

	for (uint64_t result_index = 0; result_index < result_count; result_index++)
	{
		read_recipe_set(seq);
	}
}

void deserializer::set_compressed_remainder(io::reader &reader)
{
	m_remainder_compressed_item =
		core::create_definition_from_reader(reader).with_name(core::archive::c_remainder_compressed);

	std::shared_ptr<io::reader_factory> remainder_compressed_factory =
		std::make_shared<io::basic_reader_factory>(reader);

	auto remainder_compressed_prep = std::make_shared<core::prepared_item>(
		m_remainder_compressed_item, diffs::core::prepared_item::reader_kind{remainder_compressed_factory});

	m_archive->store_item(remainder_compressed_prep);
}

void deserializer::set_remainder(const std::string &path)
{
	auto uncompressed_reader = io::file::io_device::make_reader(path);

	auto buffer_for_writer                    = std::make_shared<std::vector<char>>();
	std::shared_ptr<io::writer> buffer_writer = std::make_shared<io::buffer::writer>(buffer_for_writer);
	std::shared_ptr<io::sequential::writer> seq_writer =
		std::make_shared<io::sequential::basic_writer_wrapper>(buffer_writer);

	using init_type = io::compressed::zlib_helpers::init_type;
	io::compressed::zlib_compression_writer compression_writer(seq_writer, 9, init_type::raw);
	compression_writer.write(uncompressed_reader);
	compression_writer.flush();

	using brd                     = io::buffer::io_device;
	auto compressed_buffer_reader = brd::make_reader(buffer_for_writer, brd::size_kind::vector_size);

	std::shared_ptr<io::reader_factory> compressed_factory =
		std::make_shared<io::basic_reader_factory>(compressed_buffer_reader);

	m_remainder_compressed_item = core::create_definition_from_vector_using_size(*buffer_for_writer)
	                                  .with_name(core::archive::c_remainder_compressed);
	auto compressed_prep = std::make_shared<core::prepared_item>(
		m_remainder_compressed_item, diffs::core::prepared_item::reader_kind{compressed_factory});

	m_archive->store_item(compressed_prep);

	m_remainder_uncompressed_item =
		core::create_definition_from_reader(uncompressed_reader).with_name(core::archive::c_remainder_uncompressed);

	std::shared_ptr<io::sequential::reader_factory> uncompressed_factory =
		std::make_shared<diffs::core::zlib_decompression_reader_factory>(
			m_remainder_uncompressed_item, compressed_prep, init_type::raw);
	auto uncompressed_prep = std::make_shared<core::prepared_item>(
		m_remainder_uncompressed_item, diffs::core::prepared_item::sequential_reader_kind{uncompressed_factory});
	m_archive->store_item(uncompressed_prep);

	std::vector<uint64_t> number_ingredients;
	number_ingredients.push_back(static_cast<uint64_t>(init_type::raw));
	std::vector<core::item_definition> item_ingredients{m_remainder_compressed_item};
	std::shared_ptr<core::recipe> deflated_remainder_recipe =
		std::make_shared<diffs::recipes::compressed::zlib_decompression_recipe>(
			m_remainder_uncompressed_item, number_ingredients, item_ingredients);
	m_archive->add_recipe(deflated_remainder_recipe);
}

void deserializer::set_inline_assets(io::reader &reader)
{
	m_inline_assets_item = core::create_definition_from_reader(reader).with_name(core::archive::c_inline_assets);

	std::shared_ptr<io::reader_factory> inline_assets_factory = std::make_shared<io::basic_reader_factory>(reader);

	auto inline_assets_prep = std::make_shared<core::prepared_item>(
		m_inline_assets_item, diffs::core::prepared_item::reader_kind{inline_assets_factory});

	m_archive->store_item(inline_assets_prep);
}

void deserializer::set_inline_assets(const std::string &path)
{
	auto inline_assets_reader = io::file::io_device::make_reader(path);

	set_inline_assets(inline_assets_reader);
}

void deserializer::read_inline_assets(io::sequential::reader &seq, io::reader &reader)
{
	auto inline_assets_item = core::item_definition::read(seq, core::item_definition::serialization_options::standard)
	                              .with_name(core::archive::c_inline_assets);

	if (inline_assets_item.size() != 0)
	{
		auto inline_assets_offset = seq.tellg();
		auto inline_assets_reader = reader.slice(inline_assets_offset, inline_assets_item.size());
		set_inline_assets(inline_assets_reader);

		if (!inline_assets_item.equals(m_inline_assets_item))
		{
			throw std::exception();
		}
	}

	seq.skip(inline_assets_item.size());
}

void deserializer::read_remainder(io::sequential::reader &seq, io::reader &reader)
{
	auto remainder_compressed_item =
		core::item_definition::read(seq, core::item_definition::serialization_options::standard)
			.with_name(core::archive::c_remainder_compressed);

	if (remainder_compressed_item.size() != 0)
	{
		auto remainder_offset            = seq.tellg();
		auto remainder_compressed_reader = reader.slice(remainder_offset, remainder_compressed_item.size());
		set_compressed_remainder(remainder_compressed_reader);

		if (!remainder_compressed_item.equals(m_remainder_compressed_item))
		{
			throw std::exception();
		}

		seq.skip(remainder_compressed_item.size());
	}
}

void deserializer::read_nested_archives(io::reader &reader)
{
	uint32_t nested_archives_count;

	reader.read(0, &nested_archives_count);
	uint64_t offset = sizeof(nested_archives_count);

	for (uint32_t i = 0; i < nested_archives_count; i++)
	{
		auto remaining_length = reader.size() - offset;
		auto item_reader      = reader.slice(offset, remaining_length);
		io::sequential::basic_reader_wrapper seq(item_reader);
		auto archive_data_item =
			core::item_definition::read(seq, core::item_definition::serialization_options::standard);

		auto archive_reader_offset = seq.tellg();

		offset += archive_reader_offset;

		auto archive_reader = reader.slice(offset, archive_data_item.size());

		deserializer nested;
		nested.read(archive_reader);

		auto archive = nested.get_archive();
		m_archive->add_nested_archive(archive_data_item, archive);

		offset += archive_data_item.size();
	}
}

void deserializer::add_nested_archive(const std::string &path)
{
	auto reader            = io::file::io_device::make_reader(path);
	auto archive_data_size = reader.size();

	hashing::hash archive_data_hash{hashing::algorithm::sha256, reader};
	auto archive_data_item = core::item_definition{archive_data_size}.with_hash(archive_data_hash);

	deserializer nested;
	nested.read(reader);

	auto archive = nested.get_archive();
	m_archive->add_nested_archive(archive_data_item, archive);
}
} // namespace archive_diff::diffs::serialization::standard