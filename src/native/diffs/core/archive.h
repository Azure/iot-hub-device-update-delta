/**
 * @file archive.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include <optional>
#include <io/writer.h>

#include <json/json.h>

#include "item_definition.h"
#include "kitchen.h"

namespace archive_diff::diffs::core
{
class archive
{
	public:
	archive() = default;

	inline static const std::string c_inline_assets{"inline_assets"};
	inline static const std::string c_remainder_uncompressed{"remainder.uncompressed"};
	inline static const std::string c_remainder_compressed{"remainder.compressed"};

	void set_archive_item(const item_definition &archive_item) { m_archive_item = archive_item; }
	const item_definition &get_archive_item() const { return m_archive_item; }

	void set_source_item(const item_definition &source_item) { m_source_item = source_item; }
	const item_definition &get_source_item() const { return m_source_item; }

	void add_payload(const std::string &name, const item_definition &item) { m_payload.insert(std::pair{name, item}); }

	const std::shared_ptr<cookbook> &get_cookbook() const { return m_cookbook; }
	const std::shared_ptr<pantry> &get_pantry() const { return m_pantry; }

	void stock_kitchen(kitchen *kitchen);

	void store_item(std::shared_ptr<prepared_item> &item) { m_pantry->add(item); }
	bool try_fetch_stored_item_by_name(const std::string &name, std::shared_ptr<prepared_item> *item)
	{
		return m_pantry->find(name, item);
	}

	void add_recipe(std::shared_ptr<recipe> &recipe);
	void add_nested_archive(const item_definition &result, std::shared_ptr<archive> &nested_archive);
	bool has_nested_archive(const item_definition &result) const;
	std::vector<std::shared_ptr<archive>> get_nested_archives() const
	{
		std::vector<std::shared_ptr<archive>> archives;

		for (auto &entry : m_nested_archives)
		{
			archives.push_back(entry.second);
		}

		return archives;
	}

	std::shared_ptr<recipe> create_recipe(
		uint32_t recipe_type_id,
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients);
	bool try_get_supported_recipe_type_id(const std::string &name, uint32_t *recipe_type_id) const;
	uint32_t add_supported_recipe_type(const std::string &recipe_name);
	void set_recipe_template_for_recipe_type(
		const std::string recipe_name, std::shared_ptr<recipe_template> &recipe_template);
	size_t get_supported_recipe_type_count() const { return m_supported_recipe_templates.size(); }
	std::string get_supported_type_name(uint32_t type_id) { return m_recipe_type_id_to_type_name[type_id]; }

	Json::Value to_json() const;

	private:
	std::shared_ptr<pantry> m_pantry{std::make_shared<pantry>()};
	std::shared_ptr<cookbook> m_cookbook{std::make_shared<cookbook>()};

	std::map<item_definition, std::shared_ptr<archive>> m_nested_archives{};

	std::map<uint32_t, std::shared_ptr<recipe_template>> m_supported_recipe_templates;
	std::map<std::string, uint32_t> m_recipe_type_name_to_type_id;
	std::map<uint32_t, std::string> m_recipe_type_id_to_type_name;

	std::map<std::string, item_definition> m_payload;
	item_definition m_archive_item;
	item_definition m_source_item;
};
} // namespace archive_diff::diffs::core