/**
 * @file serializer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <limits>

#include "serializer.h"
#include "archive.h"
#include "item_definition.h"

#include <io/buffer/writer.h>
#include <io/hashed/hashed_sequential_writer.h>

#include "constants.h"

namespace archive_diff::diffs::serialization::standard
{
void serializer::write(io::sequential::writer &writer)
{
	write_header(writer);
	write_suported_recipe_types(writer);
	write_recipes(writer);
	write_inline_assets(writer);
	write_remainder(writer);
	write_nested_archives(writer);
}

void serializer::write_header(io::sequential::writer &writer)
{
	writer.write(std::string_view{g_DIFF_MAGIC_VALUE.data(), g_DIFF_MAGIC_VALUE.size()});
	writer.write_uint64_t(g_STANDARD_DIFF_VERSION_2);

	auto target_item = m_archive->get_archive_item();
	target_item.write(writer, core::item_definition::serialization_options::standard);

	auto source_item = m_archive->get_source_item();
	source_item.write(writer, core::item_definition::serialization_options::standard);
}

void serializer::write_suported_recipe_types(io::sequential::writer &writer)
{
	auto supported_recipe_type_count = m_archive->get_supported_recipe_type_count();

	if (supported_recipe_type_count > std::numeric_limits<uint32_t>::max())
	{
		throw std::exception();
	}

	writer.write_uint32_t(static_cast<uint32_t>(supported_recipe_type_count));

	for (uint32_t i = 0; i < supported_recipe_type_count; i++)
	{
		auto recipe_type_name = m_archive->get_supported_type_name(i);
		writer.write_value(recipe_type_name);
	}
}

void serializer::write_recipe_set(
	io::sequential::writer &writer, const core::item_definition &result, const core::recipe_set &recipes)
{
	uint64_t recipe_count = recipes.size();
	writer.write_uint64_t(recipe_count);

	result.write(writer, core::item_definition::serialization_options::standard);

	for (auto &recipe : recipes)
	{
		write_recipe(writer, *recipe);
	}
}

void serializer::write_recipes(io::sequential::writer &writer)
{
	auto recipe_set_map = m_archive->get_cookbook()->get_all_recipes();

	uint64_t result_set_count = recipe_set_map.size();
	writer.write_uint64_t(result_set_count);

	for (auto entry : recipe_set_map)
	{
		write_recipe_set(writer, entry.first, entry.second);
	}
}

void serializer::write_recipe(io::sequential::writer &writer, const core::recipe &recipe)
{
	auto recipe_name = recipe.get_recipe_name();
	uint32_t recipe_type_id;
	if (!m_archive->try_get_supported_recipe_type_id(recipe_name, &recipe_type_id))
	{
		throw std::exception();
	}
	writer.write_uint32_t(static_cast<uint32_t>(recipe_type_id));

	auto numbers = recipe.get_number_ingredients();
	writer.write_uint64_t(static_cast<uint64_t>(numbers.size()));
	for (auto &number : numbers)
	{
		writer.write_uint64_t(number);
	}

	auto items = recipe.get_item_ingredients();
	writer.write_uint64_t(static_cast<uint64_t>(items.size()));
	for (auto &item : items)
	{
		item.write(writer, core::item_definition::serialization_options::standard);
	}
}

void serializer::write_inline_assets(io::sequential::writer &writer)
{
	std::shared_ptr<core::prepared_item> inline_assets;
	if (!m_archive->try_fetch_stored_item_by_name(core::archive::c_inline_assets, &inline_assets))
	{
		core::item_definition empty_inline_assets{0};
		empty_inline_assets.write(writer, core::item_definition::serialization_options::standard);
		return;
	}

	auto inline_assets_item = inline_assets->get_item_definition();
	inline_assets_item.write(writer, core::item_definition::serialization_options::standard);

	auto reader = inline_assets->make_sequential_reader();
	writer.write(*reader);
}

void serializer::write_remainder(io::sequential::writer &writer)
{
	std::shared_ptr<core::prepared_item> remainder_compressed;
	if (!m_archive->try_fetch_stored_item_by_name(core::archive::c_remainder_compressed, &remainder_compressed))
	{
		core::item_definition empty_remainder_item{0};
		empty_remainder_item.write(writer, core::item_definition::serialization_options::standard);
		return;
	}

	auto remainder_compressed_item = remainder_compressed->get_item_definition();

	remainder_compressed_item.write(writer, core::item_definition::serialization_options::standard);

	auto reader = remainder_compressed->make_sequential_reader();
	writer.write(*reader);
}

void serializer::write_nested_archives(io::sequential::writer &writer)
{
	auto archives = m_archive->get_nested_archives();

	uint32_t nested_archives_count = static_cast<uint32_t>(archives.size());
	writer.write_uint32_t(nested_archives_count);

	for (auto &archive : archives)
	{
		auto serialized_diff                      = std::make_shared<std::vector<char>>();
		std::shared_ptr<io::writer> buffer_writer = std::make_shared<io::buffer::writer>(serialized_diff);
		auto hasher                               = std::make_shared<hashing::hasher>(hashing::algorithm::sha256);
		io::hashed::hashed_sequential_writer seq(buffer_writer, hasher);

		serializer nested(archive);

		nested.write(seq);

		auto item = core::item_definition{serialized_diff->size()}.with_hash(hasher->get_hash());
		item.write(writer, diffs::core::item_definition::serialization_options::standard);
		writer.write(*serialized_diff);
	}
}
} // namespace archive_diff::diffs::serialization::standard