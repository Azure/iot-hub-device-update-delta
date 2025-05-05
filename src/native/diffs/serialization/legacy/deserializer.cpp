/**
 * @file deserializer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <limits>
#include <memory>

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

#include <diffs/recipes/basic/all_zeros_recipe.h>
#include <diffs/recipes/basic/chain_recipe.h>
#include <diffs/recipes/basic/slice_recipe.h>

#include <diffs/recipes/compressed/bspatch_decompression_recipe.h>
#include <diffs/recipes/compressed/zlib_decompression_recipe.h>
#include <diffs/recipes/compressed/zstd_compression_recipe.h>
#include <diffs/recipes/compressed/zstd_decompression_recipe.h>

#include <diffs/core/zlib_decompression_reader_factory.h>

#include "legacy_recipe_type.h"

#include "deserializer.h"
#include "constants.h"

namespace archive_diff::diffs::serialization::legacy
{
// static void write_recipe_type(io::sequential::writer &writer, legacy_recipe_type type)
//{
//	if (static_cast<uint32_t>(type) >= std::numeric_limits<uint8_t>::max())
//	{
//		writer.write_value(std::numeric_limits<uint8_t>::max());
//		writer.write_value(static_cast<uint32_t>(type));
//	}
//	else
//	{
//		writer.write_value(static_cast<uint8_t>(type));
//	}
// }

legacy_recipe_type deserializer::read_recipe_type(io::sequential::reader &reader)
{
	{
		uint8_t recipe_type;
		reader.read_uint8_t(&recipe_type);

		if (recipe_type < std::numeric_limits<uint8_t>::max())
		{
			return static_cast<legacy_recipe_type>(recipe_type);
		}
	}

	legacy_recipe_type recipe_type;
	reader.read_uint32_t(reinterpret_cast<uint32_t *>(&recipe_type));
	return recipe_type;
}

namespace basic      = diffs::recipes::basic;
namespace compressed = diffs::recipes::compressed;

std::map<legacy_recipe_type, std::shared_ptr<diffs::core::recipe_template>> deserializer::m_recipe_type_to_template = {
	{legacy_recipe_type::concatentation, std::make_shared<basic::chain_recipe::recipe_template>()},
	{legacy_recipe_type::bsdiff, std::make_shared<compressed::bspatch_decompression_recipe::recipe_template>()},
	{legacy_recipe_type::zstd_compression, std::make_shared<compressed::zstd_compression_recipe::recipe_template>()},
	{legacy_recipe_type::zstd_delta, std::make_shared<compressed::zstd_decompression_recipe::recipe_template>()},
	{legacy_recipe_type::zstd_decompression,
     std::make_shared<compressed::zstd_decompression_recipe::recipe_template>()},
	{legacy_recipe_type::all_zero, std::make_shared<basic::all_zeros_recipe::recipe_template>()},
};

void deserializer::add_legacy_recipe(
	legacy_recipe_type type,
	const diffs::core::item_definition &result_item_definition,
	std::vector<uint64_t> &number_ingredients,
	std::vector<diffs::core::item_definition> &item_ingredients)
{
	if (m_recipe_type_to_template.count(type) > 0)
	{
		auto &recipe_template = m_recipe_type_to_template[type];
		auto recipe = recipe_template->create_recipe(result_item_definition, number_ingredients, item_ingredients);

		m_all_recipes.push_back(recipe);
		return;
	}

	switch (type)
	{
	case legacy_recipe_type::region: {
		if (number_ingredients.size() != 2)
		{
			throw std::exception();
		}
		auto offset                   = number_ingredients[0];
		auto slice_number_ingredients = std::vector<uint64_t>{{offset}};
		auto slice =
			std::make_shared<basic::slice_recipe>(result_item_definition, slice_number_ingredients, item_ingredients);
		m_all_recipes.emplace_back(slice);
	}
	break;
	case legacy_recipe_type::copy:
		// nothing to do!
		break;
	case legacy_recipe_type::nested: {
		m_pending_nested_diffs.push_back(
			pending_nested_diff{result_item_definition, number_ingredients, item_ingredients});
		break;
	}

	case legacy_recipe_type::remainder: {
		m_pending_remainder_slices.emplace_back(
			pending_slice{result_item_definition, m_all_recipes.size(), number_ingredients});
		m_all_recipes.push_back(std::shared_ptr<diffs::core::recipe>());
		break;
	}
	case legacy_recipe_type::inline_asset:
		m_pending_inline_assets_slices.emplace_back(
			pending_slice{result_item_definition, m_all_recipes.size(), number_ingredients});
		m_all_recipes.push_back(std::shared_ptr<diffs::core::recipe>());
		break;

	case legacy_recipe_type::inline_asset_copy:
		m_pending_inline_assets_slice_copies.emplace_back(
			pending_slice{result_item_definition, m_all_recipes.size(), number_ingredients});
		m_all_recipes.push_back(std::shared_ptr<diffs::core::recipe>());
		break;

	case legacy_recipe_type::copy_source:
		if (m_source_item.size() == 0)
		{
			throw std::exception();
		}
		else
		{
			auto offset                   = number_ingredients[0];
			auto slice_number_ingredients = std::vector<uint64_t>{{offset}};
			auto slice_item_ingredients   = std::vector<diffs::core::item_definition>{{m_source_item}};
			auto recipe                   = std::make_shared<basic::slice_recipe>(
                result_item_definition, slice_number_ingredients, slice_item_ingredients);
			m_all_recipes.push_back(recipe);
		}
		break;
	case legacy_recipe_type::gz_decompression: {
		const uint64_t c_zlib_gz_init      = 1;
		auto decompress_number_ingredients = std::vector<uint64_t>{static_cast<uint64_t>(c_zlib_gz_init)};
		auto recipe                        = std::make_shared<compressed::zlib_decompression_recipe>(
            result_item_definition, decompress_number_ingredients, item_ingredients);
		m_all_recipes.push_back(recipe);
	}
	break;

	default:
		// Unknown recipe type!
		throw std::exception();
	}
}

using prepared_item   = diffs::core::prepared_item;
using item_definition = diffs::core::item_definition;

std::string deserializer::get_decorated_name_for_origin(
	const std::string &base_name, std::optional<diffs::core::item_definition> origin)
{
	if (!origin.has_value())
	{
		return base_name;
	}

	auto find_itr = m_nested_diff_alias_map->find(origin.value());
	uint32_t index;

	if (find_itr == m_nested_diff_alias_map->cend())
	{
		if (m_nested_diff_alias_map->size() > std::numeric_limits<uint32_t>::max())
		{
			throw std::exception();
		}
		index = static_cast<uint32_t>(m_nested_diff_alias_map->size());
		m_nested_diff_alias_map->insert(std::pair{origin.value(), index});
	}
	else
	{
		index = find_itr->second;
	}

	auto nested_diff_name = "nested." + std::to_string(index);

	return base_name + "." + nested_diff_name;
}

void deserializer::create_diff_item(io::reader &reader)
{
	item_definition diff_item;
	if (m_origin.has_value())
	{
		m_diff_item = m_origin.value();
	}
	else
	{
		m_diff_item = item_definition{reader.size()}.with_name("diff");
	}

	std::shared_ptr<io::reader_factory> diff_reader_factory = std::make_shared<io::basic_reader_factory>(reader);

	m_diff_prepared_item =
		std::make_shared<prepared_item>(m_diff_item, prepared_item::reader_kind{diff_reader_factory});

	store_item(m_diff_prepared_item);
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

	m_remainder_compressed_item =
		core::create_definition_from_vector_using_size(*buffer_for_writer).with_name("remainder.compressed");
	auto compressed_prep = std::make_shared<core::prepared_item>(
		m_remainder_compressed_item, diffs::core::prepared_item::reader_kind{compressed_factory});

	m_archive->store_item(compressed_prep);

	m_remainder_uncompressed_item =
		core::create_definition_from_reader(uncompressed_reader).with_name("remainder.uncompressed");

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

void deserializer::set_inline_assets(const std::string &path)
{
	auto inline_assets_reader = io::file::io_device::make_reader(path);
	m_inline_assets_item      = core::create_definition_from_reader(inline_assets_reader).with_name("inline_assets");

	std::shared_ptr<io::reader_factory> inline_assets_factory =
		std::make_shared<io::basic_reader_factory>(inline_assets_reader);

	auto inline_assets_prep = std::make_shared<core::prepared_item>(
		m_inline_assets_item, diffs::core::prepared_item::reader_kind{inline_assets_factory});

	m_archive->store_item(inline_assets_prep);
}

void deserializer::create_remainder_items([[maybe_unused]] io::reader &reader)
{
	auto remainder_compressed_name = get_decorated_name_for_origin(core::archive::c_remainder_compressed, m_origin);

	m_remainder_compressed_item = item_definition{m_remainder_compressed_size}.with_name(remainder_compressed_name);

	auto remainder_compressed_prepared_item = std::make_shared<prepared_item>(
		m_remainder_compressed_item,
		prepared_item::slice_kind{m_remainder_offset, m_remainder_compressed_size, m_diff_prepared_item});

	auto remainder_uncompressed_name = get_decorated_name_for_origin(core::archive::c_remainder_uncompressed, m_origin);

	m_remainder_uncompressed_item =
		item_definition{m_remainder_uncompressed_size}.with_name(remainder_uncompressed_name);

	store_item(remainder_compressed_prepared_item);

	using init_type = io::compressed::zlib_helpers::init_type;
	std::shared_ptr<io::sequential::reader_factory> uncompressed_reader_factory =
		std::make_shared<core::zlib_decompression_reader_factory>(
			m_remainder_uncompressed_item, remainder_compressed_prepared_item, init_type::raw);
	auto remainder_uncompressed_prepared_item = std::make_shared<prepared_item>(
		m_remainder_uncompressed_item,
		prepared_item::sequential_reader_kind{uncompressed_reader_factory, {{remainder_compressed_prepared_item}}});

	store_item(remainder_uncompressed_prepared_item);
}

void deserializer::create_inline_assets_item([[maybe_unused]] io::reader &reader)
{
	auto inline_assets_name = get_decorated_name_for_origin(core::archive::c_inline_assets, m_origin);

	m_inline_assets_item = item_definition{m_inline_assets_size}.with_name(inline_assets_name);

	auto inline_assets_prepared_item = std::make_shared<prepared_item>(
		m_inline_assets_item,
		prepared_item::slice_kind{m_inline_assets_offset, m_inline_assets_size, m_diff_prepared_item});

	store_item(inline_assets_prepared_item);
}

void deserializer::process_remainder_and_inline_assets()
{
	std::vector<uint64_t> number_ingredients{0};
	std::vector<item_definition> item_ingredients{m_remainder_uncompressed_item};

	uint64_t remainder_total_written{0};

	for (auto &pending_remainder_slice : m_pending_remainder_slices)
	{
		auto &result_item_definition = pending_remainder_slice.result_item_definition;
		auto index                   = pending_remainder_slice.offset_in_all_recipes;

		m_all_recipes[index] = std::make_shared<diffs::recipes::basic::slice_recipe>(
			result_item_definition, number_ingredients, item_ingredients);

		// printf(
		//	"Setting up remainder at offset: %d, item: %s\n",
		//	(int)number_ingredients[0],
		//	result_item_definition.to_string().c_str());

		// printf("Remainder total written = %d\n", (int)remainder_total_written);
		remainder_total_written += result_item_definition.size();

		number_ingredients[0] += result_item_definition.size();
	}

	number_ingredients[0] = 0;
	item_ingredients[0]   = m_inline_assets_item;
	for (auto &pending_inline_asset_item : m_pending_inline_assets_slices)
	{
		auto &result_item_definition = pending_inline_asset_item.result_item_definition;
		auto index                   = pending_inline_asset_item.offset_in_all_recipes;

		m_all_recipes[index] = std::make_shared<diffs::recipes::basic::slice_recipe>(
			result_item_definition, number_ingredients, item_ingredients);

		number_ingredients[0] += result_item_definition.size();
	}

	for (auto &pending_inline_asset_item : m_pending_inline_assets_slice_copies)
	{
		auto &result_item_definition  = pending_inline_asset_item.result_item_definition;
		auto index                    = pending_inline_asset_item.offset_in_all_recipes;
		auto &number_ingredients_copy = pending_inline_asset_item.number_ingredients;

		m_all_recipes[index] = std::make_shared<diffs::recipes::basic::slice_recipe>(
			result_item_definition, number_ingredients_copy, item_ingredients);
	}
}

void deserializer::finalize_legacy_recipes()
{
	//
	// Fix up the remainder/inline asset slice recipes
	//
	process_remainder_and_inline_assets();

	//
	// Place items into the cookbook
	//
	populate_cookbook();

	//
	// Lastly, process any pending nested diffs so we get them included
	//
	process_nested_diffs();
}

void deserializer::populate_cookbook()
{
	for (auto &recipe : m_all_recipes)
	{
		add_recipe(recipe);
	}
}

void deserializer::process_nested_diffs()
{
	for (auto &pending_nested_diff : m_pending_nested_diffs)
	{
		auto &result = pending_nested_diff.result_item_definition;
		if (m_archive->has_nested_archive(result))
		{
			continue;
		}

		auto &items = pending_nested_diff.item_ingredients;

		if (items.size() != 2)
		{
			throw std::exception();
		}
		auto &diff_item = items[0];

		std::shared_ptr<core::kitchen> kitchen = core::kitchen::create();
		m_archive->stock_kitchen(kitchen.get());
		kitchen->request_item(diff_item);
		if (!kitchen->process_requested_items())
		{
			throw std::exception();
		}
		auto diff_prep   = kitchen->fetch_item(diff_item);
		auto diff_reader = diff_prep->make_reader();

		deserializer deserializer{diff_item, m_nested_diff_alias_map};

		std::string reason;
		if (!deserializer.is_this_format(diff_reader, &reason))
		{
			throw std::exception();
		}

		deserializer.read(diff_reader);

		auto nested_archive = deserializer.get_archive();

		m_archive->add_nested_archive(result, nested_archive);
	}
}

bool deserializer::is_this_format(io::reader &reader, std::string *reason)
{
	const auto c_header_size = (g_DIFF_MAGIC_VALUE.size() + sizeof(g_DIFF_VERSION));
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

	reader.read_uint64_t(4, &version);

	if (version != g_DIFF_VERSION)
	{
		*reason = "Wrong version. Expected: " + std::to_string(g_DIFF_VERSION) + ", Found: " + std::to_string(version);
		return false;
	}

	return true;
}

void deserializer::read(io::reader &reader)
{
	io::sequential::basic_reader_wrapper seq(reader);

	char magic[4]{};

	seq.read(std::span<char>{magic, sizeof(magic)});

	if (0 != std::memcmp(magic, g_DIFF_MAGIC_VALUE.data(), sizeof(magic)))
	{
		std::string magic_string(magic, 4);
		std::string msg = "Not a valid diff file - invalid magic. Found: " + magic_string;
		throw errors::user_exception(errors::error_code::diff_magic_header_wrong, msg);
	}

	uint64_t version;

	seq.read_uint64_t(&version);

	if (version != g_DIFF_VERSION)
	{
		std::string msg =
			"Not a valid version. Expected: " + std::to_string(g_DIFF_VERSION) + ", Found: " + std::to_string(version);
		throw errors::user_exception(errors::error_code::diff_version_wrong, msg);
	}

	uint64_t length;
	seq.read_uint64_t(&length);

	hashing::hash archive_item_hash;
	archive_item_hash.read(seq);
	auto archive_item = diffs::core::item_definition{length}.with_hash(archive_item_hash);
	if (m_archive.get())
	{
		m_archive->set_archive_item(archive_item);
	}

	seq.read_uint64_t(&length);
	if (length != 0)
	{
		hashing::hash source_item_hash;
		source_item_hash.read(seq);
		auto source_item = diffs::core::item_definition{length}.with_hash(source_item_hash);
		m_archive->set_source_item(source_item);
		m_source_item = source_item;
	}

	uint64_t chunk_count;
	seq.read_uint64_t(&chunk_count);

	std::vector<diffs::core::item_definition> chain_ingredients;

	for (uint64_t i = 0; i < chunk_count; i++)
	{
		chain_ingredients.emplace_back(read_chunk(seq));
	}

	m_all_recipes.emplace_back(std::make_shared<diffs::recipes::basic::chain_recipe>(
		archive_item, std::vector<uint64_t>(), chain_ingredients));

	seq.read_uint64_t(&m_inline_assets_size);
	m_inline_assets_offset = seq.tellg();
	seq.skip(m_inline_assets_size);

	seq.read_uint64_t(&m_remainder_uncompressed_size);
	seq.read_uint64_t(&m_remainder_compressed_size);
	m_remainder_offset = seq.tellg();

	m_diff_size = m_remainder_offset + m_remainder_compressed_size;

	if (m_diff_size != seq.size())
	{
		std::string msg = "diffs::read(). Size mismatch for diff. Size based on reading data: "
		                + std::to_string(m_diff_size) + ". Size from reader: " + std::to_string(reader.size());
		throw errors::user_exception(errors::error_code::diff_read_diff_size_mismatch, msg);
	}

	//
	// Create core items
	//
	create_diff_item(reader);
	create_remainder_items(reader);
	create_inline_assets_item(reader);

	finalize_legacy_recipes();
}

diffs::core::item_definition deserializer::read_chunk(io::sequential::reader &reader)
{
	uint64_t length;
	reader.read_uint64_t(&length);
	hashing::hash hash;
	hash.read(reader);
	auto item = diffs::core::item_definition{length}.with_hash(hash);

	read_recipe(item, reader);

	return item;
}

diffs::core::item_definition deserializer::read_archive_item(io::sequential::reader &reader)
{
	legacy_archive_item_type type;
	reader.read_uint8_t(reinterpret_cast<uint8_t *>(&type));
	if (type == legacy_archive_item_type::chunk)
	{
		uint64_t offset;
		reader.read_uint64_t(&offset);
	}
	uint64_t length;
	reader.read_uint64_t(&length);
	hashing::hash hash;
	hash.read(reader);
	auto item = diffs::core::item_definition{length}.with_hash(hash);

	uint8_t has_recipe;
	reader.read_uint8_t(&has_recipe);

	if (has_recipe)
	{
		read_recipe(item, reader);
	}
	return item;
}
enum class recipe_parameter_type : uint8_t
{
	archive_item = 0,
	number       = 1,
};

void deserializer::read_recipe(
	const diffs::core::item_definition &result_item_definition, io::sequential::reader &reader)
{
	auto recipe_type = read_recipe_type(reader);

	std::vector<uint64_t> number_ingredients;
	std::vector<diffs::core::item_definition> item_ingredients;

	uint8_t parameter_count;
	reader.read_uint8_t(&parameter_count);

	for (uint8_t i = 0; i < parameter_count; i++)
	{
		recipe_parameter_type param_type;
		reader.read_uint8_t(reinterpret_cast<uint8_t *>(&param_type));

		switch (param_type)
		{
		case recipe_parameter_type::number:
			uint64_t number_value;
			reader.read_uint64_t(&number_value);
			number_ingredients.push_back(number_value);
			break;
		case recipe_parameter_type::archive_item: {
			auto item = read_archive_item(reader);
			item_ingredients.push_back(item);
		}
		break;
		default:
			std::string msg =
				"diffs::recipe_parameter::read(): Invalid type: " + std::to_string(static_cast<int>(param_type));
			throw errors::user_exception(errors::error_code::diff_recipe_parameter_read_invalid_type, msg);
		}
	}

	add_legacy_recipe(recipe_type, result_item_definition, number_ingredients, item_ingredients);
}
} // namespace archive_diff::diffs::serialization::legacy