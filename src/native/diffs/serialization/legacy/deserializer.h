/**
 * @file deserializer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/reader.h>

#include <diffs/core/archive.h>
#include <diffs/core/recipe_template.h>

#include "legacy_recipe_type.h"

namespace archive_diff::diffs::serialization::legacy
{
class deserializer
{
	public:
	deserializer() = default;
	deserializer(
		const diffs::core::item_definition &origin,
		std::shared_ptr<std::map<core::item_definition, uint32_t>> &alias_map) :
		m_origin(origin), m_nested_diff_alias_map(alias_map)
	{}

	static bool is_this_format(io::reader &reader, std::string *reason);

	void read(io::reader &reader);

	void set_target_item(const core::item_definition &item) { m_archive->set_archive_item(item); }
	void set_source_item(const core::item_definition &item)
	{
		m_source_item = item;
		m_archive->set_source_item(item);
	}

	void add_legacy_recipe(
		legacy_recipe_type type,
		const diffs::core::item_definition &result_item_definition,
		std::vector<uint64_t> &number_ingredients,
		std::vector<diffs::core::item_definition> &item_ingredients);

	void finalize_legacy_recipes();

	void store_item(std::shared_ptr<core::prepared_item> &item_prep) { m_archive->store_item(item_prep); }
	void add_recipe(std::shared_ptr<core::recipe> &recipe) { m_archive->add_recipe(recipe); }
	void add_payload(const std::string &name, const core::item_definition &item) { m_archive->add_payload(name, item); }

	void set_remainder(const std::string &path);
	void set_inline_assets(const std::string &path);

	std::shared_ptr<diffs::core::archive> get_archive() const { return m_archive; }

	uint64_t get_inline_assets_size() const { return m_inline_assets_size; }
	uint64_t get_inline_assets_offset() const { return m_inline_assets_offset; }

	uint64_t get_remainder_uncompressed_size() const { return m_remainder_uncompressed_size; }
	uint64_t get_remainder_compressed_size() const { return m_remainder_compressed_size; }
	uint64_t get_remainder_offset() const { return m_remainder_offset; }

	uint64_t get_diff_size() const { return m_diff_size; }

	private:
	enum class legacy_archive_item_type : uint8_t
	{
		blob    = 0,
		chunk   = 1,
		payload = 2,
	};

	diffs::core::item_definition read_chunk(io::sequential::reader &reader);
	diffs::core::item_definition read_archive_item(io::sequential::reader &reader);
	void read_recipe(const diffs::core::item_definition &result_item_definition, io::sequential::reader &reader);

	static legacy_recipe_type read_recipe_type(io::sequential::reader &reader);

	static std::map<legacy_recipe_type, std::shared_ptr<diffs::core::recipe_template>> m_recipe_type_to_template;

	private:
	std::map<core::item_definition, uint64_t> m_written_chunks;

	using legacy_recipe_parameters = std::vector<std::variant<uint64_t, core::item_definition>>;

	private:
	diffs::core::item_definition m_source_item{};

	diffs::core::item_definition m_diff_item{};
	std::shared_ptr<diffs::core::prepared_item> m_diff_prepared_item;

	diffs::core::item_definition m_inline_assets_item{};
	diffs::core::item_definition m_remainder_uncompressed_item{};
	diffs::core::item_definition m_remainder_compressed_item{};

	std::shared_ptr<diffs::core::archive> m_archive{std::make_shared<diffs::core::archive>()};
	std::optional<diffs::core::item_definition> m_origin;

	uint64_t m_inline_assets_size{};
	uint64_t m_inline_assets_offset{};

	uint64_t m_remainder_uncompressed_size{};
	uint64_t m_remainder_compressed_size{};
	uint64_t m_remainder_offset{};

	uint64_t m_diff_size{};

	std::set<size_t> m_chunk_recipe_indices;
	std::vector<std::shared_ptr<diffs::core::recipe>> m_all_recipes{};

	struct pending_slice
	{
		diffs::core::item_definition result_item_definition;
		size_t offset_in_all_recipes;
		std::vector<uint64_t> number_ingredients;
	};

	std::vector<pending_slice> m_pending_inline_assets_slices;
	std::vector<pending_slice> m_pending_inline_assets_slice_copies;
	std::vector<pending_slice> m_pending_remainder_slices;

	std::string get_decorated_name_for_origin(
		const std::string &base_name, std::optional<diffs::core::item_definition> origin);

	void create_diff_item(io::reader &reader);
	void create_source_and_target_items(io::reader &reader);
	void create_remainder_items(io::reader &reader);
	void create_inline_assets_item(io::reader &reader);

	void process_remainder_and_inline_assets();
	void populate_cookbook();
	void process_nested_diffs();

	struct pending_nested_diff
	{
		diffs::core::item_definition result_item_definition;
		std::vector<uint64_t> number_ingredients;
		std::vector<diffs::core::item_definition> item_ingredients;
	};

	std::vector<pending_nested_diff> m_pending_nested_diffs;

	std::shared_ptr<std::map<core::item_definition, uint32_t>> m_nested_diff_alias_map{
		std::make_shared<std::map<core::item_definition, uint32_t>>()};
};
} // namespace archive_diff::diffs::serialization::legacy