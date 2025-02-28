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

#include <diffs/serialization/standard/builtin_recipe_types.h>

namespace archive_diff::diffs::serialization::standard
{
class deserializer
{
	public:
	deserializer() { ensure_builtin_recipe_types(m_archive.get()); }

	deserializer(
		const diffs::core::item_definition &origin,
		std::shared_ptr<std::map<core::item_definition, uint32_t>> &alias_map) :
		m_origin(origin), m_nested_diff_alias_map(alias_map)
	{
		ensure_builtin_recipe_types(m_archive.get());
	}

	static bool is_this_format(io::reader &reader, std::string *reason);

	void read(io::reader &reader);

	void set_target_item(const core::item_definition &item) { m_archive->set_archive_item(item); }
	void set_source_item(const core::item_definition &item)
	{
		m_source_item = item;
		m_archive->set_source_item(item);
	}

	void store_item(std::shared_ptr<core::prepared_item> &item_prep) { m_archive->store_item(item_prep); }
	void add_recipe(std::shared_ptr<core::recipe> &recipe) { m_archive->add_recipe(recipe); }
	void add_recipe(
		const std::string &recipe_name,
		const core::item_definition &result_item,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<core::item_definition> &item_ingredients);
	void add_payload(const std::string &name, const core::item_definition &item) { m_archive->add_payload(name, item); }

	void set_remainder(const std::string &path);
	void set_compressed_remainder(io::reader &reader);

	void set_inline_assets(const std::string &path);
	void set_inline_assets(io::reader &reader);

	void add_nested_archive(const std::string &path);

	std::shared_ptr<diffs::core::archive> get_archive() const { return m_archive; }

	private:
	void read_header(io::sequential::reader &seq);
	void read_supported_recipe_types(io::sequential::reader &seq);
	void read_recipes(io::sequential::reader &seq);
	void read_recipe_set(io::sequential::reader &seq);
	void read_recipe(io::sequential::reader &seq, const core::item_definition &result_item);
	void read_inline_assets(io::sequential::reader &seq, io::reader &reader);
	void read_remainder(io::sequential::reader &seq, io::reader &reader);
	void read_nested_archives(io::reader &reader);

	std::shared_ptr<diffs::core::archive> m_archive{std::make_shared<diffs::core::archive>()};
	std::optional<diffs::core::item_definition> m_origin;

	std::shared_ptr<std::map<core::item_definition, uint32_t>> m_nested_diff_alias_map{
		std::make_shared<std::map<core::item_definition, uint32_t>>()};

	core::item_definition m_source_item;

	core::item_definition m_inline_assets_item{};
	core::item_definition m_remainder_uncompressed_item{};
	core::item_definition m_remainder_compressed_item{};
};
} // namespace archive_diff::diffs::serialization::standard