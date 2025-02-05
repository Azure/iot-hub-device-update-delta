/**
 * @file kitchen.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "kitchen.h"
#include "adu_log.h"

#include "recipe.h"
#include <hashing/hasher.h>
#include <io/buffer/reader_factory.h>
#include <io/sequential/basic_writer_wrapper.h>

namespace archive_diff::diffs::core
{
//
// Places a recipe in the cookbook
// Invalidates 'unreachable' items.
//
// Acquires and holds m_item_request_mutex
void kitchen::add_recipe(std::shared_ptr<recipe> &recipe)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	m_unreachable_items.clear();

	m_cookbook->add_recipe(recipe);
}

//
// Adds a pantry to m_all_pantries
// Invalidates 'unreachable' items.
//
// Acquires and holds m_item_request_mutex
//
void kitchen::add_pantry(std::shared_ptr<pantry> &pantry)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	m_unreachable_items.clear();

	m_all_pantries.push_back(pantry);
}

//
// Adds a cookbook to m_all_cookbooks
// Invalidates 'unreachable' items.
//
// Acquires and holds m_item_request_mutex
//
void kitchen::add_cookbook(std::shared_ptr<cookbook> &cookbook)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	m_unreachable_items.clear();

	m_all_cookbooks.push_back(cookbook);
}

//
// Clears all requested items
//
// Acquires and holds m_item_request_mutex
void kitchen::clear_requested_items()
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	m_requested_items.clear();
}

//
// Adds an entry into 'm_requested_items' if not present within
// m_ready_items.
//
// Acquires and holds m_item_request_mutex
void kitchen::request_item(const item_definition &item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	m_requested_items.insert(item);
}

// Looks at every item in m_requested_items and tries to populate
// them into m_ready_items. any items that are not able to be prepared
// will be within m_unreachable_items.
//
// After this call has finished, the user may expect to get results from
// can_fetch_item(); any entries in m_ready_items will yield 'true',
// while any entries in m_unreachable_items will yield 'false.'
// Similarly, fetch_item() can be used to retrieve entries.
//
// Acquires and holds m_item_request_mutex
//
// Returns true if all items were fully available, false otherwise
bool kitchen::process_requested_items(bool select_recipes_only, std::set<core::item_definition> &mocked_items)
{
	bool all_requests_fulfilled{true};

	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	m_unreachable_items.clear();

	for (auto itr = m_requested_items.begin(); itr != m_requested_items.end();)
	{
		std::set<item_definition> already_using;
		if (!make_dependency_ready(*itr, select_recipes_only, mocked_items, already_using))
		{
			all_requests_fulfilled = false;
			m_unreachable_items.insert(*itr);

			// for (auto &cookbook : m_all_cookbooks)
			//{
			//	cookbook->audit_for_size(itr->size());
			// }
			ADU_LOG("Can't process item: {}", itr->to_string());
			itr++;
		}
		else
		{
			itr = m_requested_items.erase(itr);
		}
	}

	return all_requests_fulfilled;
}

bool kitchen::process_requested_items() 
{
	std::set<item_definition> mocked_items;
	return process_requested_items(false, mocked_items); 
}

bool kitchen::make_dependency_ready(
	const item_definition &item,
	bool select_recipes_only,
	std::set<item_definition> &mocked_items,
	std::set<item_definition> &already_using)
{
	if (mocked_items.contains(item))
	{
		return true;
	}
	
	if (m_selected_recipes.contains(item))
	{
		return true;
	}

	if (m_ready_items.contains(item))
	{
		return true;
	}

	if (m_unreachable_items.contains(item))
	{
		ADU_LOG("Item already determined unreachable: {}", item.to_string());
		return false;
	}

	for (auto &pantry : m_all_pantries)
	{
		std::shared_ptr<prepared_item> from_pantry;
		if (pantry->find(item, &from_pantry))
		{
			m_ready_items.insert(std::pair{item, from_pantry});
			return true;
		}
	}

	// Only check this after we've looked in the pantry to allow us to
	// use the pantry multiple times when only selecting recipes.
	if (already_using.contains(item))
	{
		return false;
	}
	else
	{
		already_using.insert(item);
	}

	for (auto &cookbook : m_all_cookbooks)
	{
		const recipe_set *recipes_for_item;
		if (!cookbook->find_recipes_for_item(item, &recipes_for_item))
		{
			continue;
		}

		for (auto &recipe : *recipes_for_item)
		{
			bool found_impossible_ingredient{false};
			item_definition impossible_item;

			std::vector<std::shared_ptr<prepared_item>> prepared_ingredients;

			auto &ingredients = recipe->get_item_ingredients();

			for (auto &ingredient : ingredients)
			{
				if (!make_dependency_ready(ingredient, select_recipes_only, mocked_items, already_using))
				{
					impossible_item = ingredient;

					ADU_LOG("Found impossible item {}, for: {}", ingredient.to_string(), item.to_string());

					found_impossible_ingredient = true;
					break;
				}
				prepared_ingredients.push_back(m_ready_items[ingredient]);
			}

			if (found_impossible_ingredient)
			{
				for (auto &ingredient : ingredients)
				{
					if (ingredient == impossible_item)
					{
						break;
					}
					already_using.erase(ingredient);
				}
				continue;
			}

			m_selected_recipes[item] = recipe;

			if (!select_recipes_only)
			{
				auto from_recipe = recipe->prepare(this, prepared_ingredients);
				m_ready_items.insert(std::pair{item, from_recipe});
			}

			return true;
		}
	}

	ADU_LOG("Couldn't make dependency: {}", item.to_string());

	return false;
}

// Looks at m_ready_items. If the item is present, will  return the contained
// prepared item. Otherwise, throws an exception.
//
// Acquires and holds m_item_request_mutex
std::shared_ptr<prepared_item> kitchen::fetch_item(const item_definition &item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	if (m_ready_items.count(item) == 0)
	{
		throw errors::user_exception(
			errors::error_code::diffs_kitchen_item_not_ready_to_fetch,
			"kitchen::fetch_item: Item not ready: " + item.to_string());
	}

	return m_ready_items[item];
}

// Looks at m_ready_items. If the item is present, will return true,
// otherwise, return false.
//
// Acquires and holds m_item_request_mutex
bool kitchen::can_fetch_item(const item_definition &item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	return m_ready_items.count(item) > 0;
}

//
// Looks at m_selected_recipes. If the item is present, will return the contained
// prepared item. Otherwise, throws an exception.
//
// Acquires and holds m_item_request_mutex
std::shared_ptr<recipe> kitchen::fetch_selected_recipe(const item_definition &item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	if (m_selected_recipes.count(item) == 0)
	{
		throw errors::user_exception(
			errors::error_code::diffs_kitchen_no_selected_recipes, "No selected recipes for: " + item.to_string());
	}

	return m_selected_recipes[item];
}

//
// Looks at m_selected_recipes. If the item is present, will return true,
// otherwise, return false.
//
// Acquires and holds m_item_request_mutex
bool kitchen::can_fetch_selected_recipe(const item_definition &item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	return m_selected_recipes.count(item) > 0;
}

// Places an item in m_ready_items
// Invalidates 'unreachable' items.
//
// Acquires and holds m_item_request_mutex
void kitchen::store_item(std::shared_ptr<prepared_item> &prepared)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);
	m_unreachable_items.clear();

	m_pantry->add(prepared);
}

//
// Gets an item from the pantry by name, if it exists and returns true,
// otherwise, returns false.
//
// Acquires and holds m_item_request_mutex
bool kitchen::try_fetch_stored_item_by_name(const std::string &name, std::shared_ptr<prepared_item> *item)
{
	std::lock_guard<std::mutex> lock(m_item_request_mutex);

	for (auto &pantry : m_all_pantries)
	{
		if (pantry->find(name, item))
		{
			return true;
		}
	}

	return false;
}

void kitchen::write_item(io::writer &writer, const item_definition &item)
{
	if (!can_fetch_item(item))
	{
		throw errors::user_exception(
			errors::error_code::diffs_kitchen_item_not_ready_to_fetch,
			"kitchen::write_item: Can't fetch item:" + item.to_string());
	}

	auto prep_result = fetch_item(item);
	ADU_LOG("prep_result: {}", prep_result->to_string());

	auto sequential_reader = prep_result->make_sequential_reader();

	auto remaining = sequential_reader->size();

	const size_t block_size = 8 * 1024;
	char data[block_size];
	uint64_t offset{0};

	while (remaining)
	{
		auto to_read = static_cast<size_t>(std::min<uint64_t>(block_size, remaining));
		sequential_reader->read(std::span<char>{data, to_read});
		writer.write(offset, std::string_view{data, to_read});

		remaining -= to_read;
		offset += to_read;
	}
}

void kitchen::save_selected_recipes(std::shared_ptr<io::writer> &writer) const
{
	io::sequential::basic_writer_wrapper seq_writer(writer);

	Json::Value recipe_list;
	Json::Value recipes;

	for (const auto& [result, recipe] : m_selected_recipes)
	{
		recipes.append(recipe->to_json());
	}

	recipe_list["Recipes"] = recipes;

	auto json_text = recipe_list.toStyledString();

	seq_writer.write_text(json_text);
}
} // namespace archive_diff::diffs::core