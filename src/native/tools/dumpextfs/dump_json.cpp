/**
 * @file dump_json.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include <string>
#include <map>
#include <set>
#include <vector>
#include <iterator>

#include "ext2fs/ext2_types.h"
#include "et/com_err.h"
#include "ext2fs/ext2_io.h"
#include <ext2fs/ext2_ext_attr.h>
#include "ext2fs/ext2fs.h"

#include <json/json.h>

#include "file_details.h"

const int c_sha256_type = 32780;

void add_hashes(Json::Value &value, const std::string &sha256_hash_string)
{
	Json::Value hashes;

	Json::Value sha256Hash;
	sha256Hash["type"] = c_sha256_type;

	sha256Hash["value"] = sha256_hash_string;
	hashes["Sha256"]    = sha256Hash;

	value["Hashes"] = hashes;
}

Json::Value create_archive_item_value(uint64_t length, const std::string &hash_sha256_string)
{
	Json::Value value;
	value["Length"] = length;

	add_hashes(value, hash_sha256_string);

	return value;
}

void add_payload(const std::string &full_path, Json::Value &payload_item, Json::Value &allPayload)
{
	Json::Value payload;
	Json::Value payloadEntry;

	payload["Origin"] = Json::nullValue;
	payload["Name"]   = full_path;

	payloadEntry["Key"] = payload;
	payloadEntry["Value"].append(payload_item);

	allPayload.append(payloadEntry);
}

void add_payload_recipe(
	const file_details &details,
	[[maybe_unused]] const Json::Value &payload_item,
	std::map<Json::Value, std::vector<Json::Value>> &recipes_map)
{
	Json::Value result = create_archive_item_value(details.length, details.hash_sha256_string);

	// Don't add payload recipe if it's a single item or less.
	if (details.regions.size() <= 1)
	{
		return;
	}

	Json::Value recipe;
	recipe["Name"]   = "chain";
	recipe["Result"] = result;

	for (auto &region : details.regions)
	{
		Json::Value item = create_archive_item_value(region.length, region.hash_sha256_string);
		recipe["ItemIngredients"].append(item);
	}

	if (recipes_map.count(result) == 0)
	{
		recipes_map.insert(std::pair{result, std::vector<Json::Value>{recipe}});
	}
	else
	{
		recipes_map[result].push_back(recipe);
	}
}

class json_entries
{
	public:
	json_entries(const archive_details &details)
	{
		m_archive_item["Length"] = details.length;
		add_hashes(m_archive_item, details.hash_sha256_string);

		for (auto &file : details.files)
		{
			import_file(file);
		}
	}

	const Json::Value &get_archive_item() const { return m_archive_item; }

	Json::Value get_recipes() const
	{
		Json::Value recipes;

		for (auto &entry : m_recipes_map)
		{
			auto &entry_recipes = entry.second;
			for (auto &recipe : entry_recipes)
			{
				recipes.append(recipe);
			}
		}

		return recipes;
	}

	Json::Value get_forward_recipes() const
	{
		Json::Value forward_recipes;

		for (auto &entry : m_forward_recipes_map)
		{
			auto &entry_recipe = entry.second;
			forward_recipes.append(entry_recipe);
		}

		return forward_recipes;
	}

	Json::Value get_reverse_recipes() const
	{
		Json::Value reverse_recipes;

		for (auto &entry : m_reverse_recipes_map)
		{
			auto &entry_recipe = entry.second;
			reverse_recipes.append(entry_recipe);
		}

		return reverse_recipes;
	}

	Json::Value get_payload() const
	{
		Json::Value payload;

		for (auto &entry : m_payload)
		{
			const std::string &name = entry.first;
			const auto &items       = entry.second;

			Json::Value kvpair_value;
			Json::Value key;
			key["ArchiveItem"] = m_archive_item;
			key["Name"]        = name;

			kvpair_value["Key"]   = key;
			kvpair_value["Value"] = items;

			payload.append(kvpair_value);
		}

		return payload;
	}

	private:
	void import_file(const file_details &file)
	{
		Json::Value payload_item = create_archive_item_value(file.length, file.hash_sha256_string);

		uint64_t offset_in_payload = 0;
		for (auto &region : file.regions)
		{
			import_region(file, region, payload_item, offset_in_payload);
			offset_in_payload += region.length;
		}

		Json::Value payload_recipe = create_payload_recipe(payload_item, file);

		if (payload_recipe != Json::nullValue)
		{
			add_recipe(payload_item, payload_recipe);
		}

		if (m_payload.find(file.full_path) == m_payload.cend())
		{
			m_payload.emplace(file.full_path, Json::Value{});
		}

		m_payload[file.full_path].append(payload_item);
	}

	void import_region(
		const file_details &file,
		const file_region &region,
		const Json::Value &payload_item,
		uint64_t offset_in_payload)
	{
		if (region.offset.has_value())
		{
			auto offset = region.offset.value();

			if (m_chunks.find(offset) != m_chunks.cend())
			{
				return;
			}

			m_chunks.emplace(offset, region);
		}

		auto chunk_result_and_recipe = create_chunk_recipe(m_archive_item, region);
		add_reverse_recipe(chunk_result_and_recipe.first, chunk_result_and_recipe.second);

		if (file.regions.size() > 1)
		{
			auto region_key = region.hash_sha256_string;

			auto result_and_recipe = create_slice_of_payload_recipe(region, payload_item, offset_in_payload);

			add_reverse_recipe(result_and_recipe.first, result_and_recipe.second);
		}
	}

	static Json::Value create_payload_recipe(const Json::Value &result_item, const file_details &file)
	{
		if (file.regions.size() < 2)
		{
			return Json::nullValue;
		}

		Json::Value recipe;

		recipe["Name"]   = "chain";
		recipe["Result"] = result_item;

		for (const auto &region : file.regions)
		{
			Json::Value item = create_archive_item_value(region.length, region.hash_sha256_string);

			recipe["ItemIngredients"].append(item);
		}

		return recipe;
	}

	static std::pair<Json::Value, Json::Value> create_chunk_recipe(
		const Json::Value &archive_item, const file_region &region)
	{
		Json::Value recipe;
		Json::Value result = create_archive_item_value(region.length, region.hash_sha256_string);

		if (region.all_zeroes)
		{
			recipe["Name"] = "all_zeros";
		}
		else
		{
			recipe["Name"] = "slice";
		}

		recipe["Result"] = result;

		if (region.all_zeroes)
		{
			recipe["NumberIngredients"].append(region.length);
		}
		else
		{
			auto offset = region.offset.value();
			recipe["NumberIngredients"].append(offset);
			recipe["ItemIngredients"].append(archive_item);
		}

		return std::make_pair(result, recipe);
	}

	static std::pair<Json::Value, Json::Value> create_slice_of_payload_recipe(
		const file_region &region, const Json::Value &payload_item, uint64_t offset)
	{
		Json::Value recipe;
		Json::Value result = create_archive_item_value(region.length, region.hash_sha256_string);

		recipe["Name"]   = "slice";
		recipe["Result"] = result;
		recipe["NumberIngredients"].append(offset);
		recipe["ItemIngredients"].append(payload_item);

		return std::make_pair(result, recipe);
	}

	void add_recipe(const Json::Value &result, const Json::Value &recipe)
	{
		if (m_recipes_map.find(result) == m_recipes_map.cend())
		{
			m_recipes_map.emplace(result, std::set<Json::Value>{});
		}

		m_recipes_map[result].insert(recipe);
	}

	void add_forward_recipe(const Json::Value &result, const Json::Value &recipe)
	{
		m_forward_recipes_map.emplace(result, recipe);
		add_recipe(result, recipe);
	}

	void add_reverse_recipe(const Json::Value &result, const Json::Value &recipe)
	{
		m_reverse_recipes_map.emplace(result, recipe);
		add_recipe(result, recipe);
	}

	private:
	Json::Value m_archive_item;

	std::map<Json::Value, std::set<Json::Value>> m_recipes_map;
	std::map<Json::Value, Json::Value> m_forward_recipes_map;
	std::map<Json::Value, Json::Value> m_reverse_recipes_map;

	std::map<uint64_t, file_region> m_chunks;

	std::map<std::string, Json::Value> m_payload;
};

void dump_archive(std::ostream &stream, const archive_details &details)
{
	Json::Value root;
	root["Type"]    = "ext4";
	root["Subtype"] = "ext4";

	// Add archive item
	json_entries entries(details);

	root["ArchiveItem"]    = entries.get_archive_item();
	root["Recipes"]        = entries.get_recipes();
	root["ForwardRecipes"] = entries.get_forward_recipes();
	root["ReverseRecipes"] = entries.get_reverse_recipes();
	root["Payload"]        = entries.get_payload();

	Json::StreamWriterBuilder builder;
	const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(root, &stream);
}
