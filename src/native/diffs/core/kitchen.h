/**
 * @file kitchen.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>
#include <mutex>
#include <memory>

#include <optional>

#include "item_definition.h"
#include "prepared_item.h"
#include "pantry.h"
#include "cookbook.h"
#include "slicer.h"
#include "recipe.h"
#include "recipe_template.h"

namespace archive_diff::diffs::core
{
class recipe;

class kitchen : public std::enable_shared_from_this<kitchen>
{
	protected:
	kitchen() = default;

	public:
	static std::shared_ptr<kitchen> create() { return std::shared_ptr<kitchen>(new kitchen()); }

	std::weak_ptr<kitchen> get_weak() { return weak_from_this(); }

	//
	// Places an item in the pantry
	// Invalidates 'unreachable' items.
	//
	// Acquires and holds m_item_request_mutex
	void store_item(std::shared_ptr<prepared_item> &prepared);

	//
	// Gets an item from the pantry by name, if it exists and returns true,
	// otherwise, returns false.
	//
	// Acquires and holds m_item_request_mutex
	bool try_fetch_stored_item_by_name(const std::string &name, std::shared_ptr<prepared_item> *item);

	//
	// Places a recipe in the cookbook
	// Invalidates 'unreachable' items.
	//
	// Acquires and holds m_item_request_mutex
	void add_recipe(std::shared_ptr<recipe> &recipe);

	//
	// Adds a pantry to m_all_pantries
	// Invalidates 'unreachable' items.
	//
	// Acquires and holds m_item_request_mutex
	//
	void add_pantry(std::shared_ptr<pantry> &pantry);

	//
	// Adds a cookbook to m_all_cookbooks
	// Invalidates 'unreachable' items.
	//
	// Acquires and holds m_item_request_mutex
	//
	void add_cookbook(std::shared_ptr<cookbook> &cookbook);

	//
	// Clears all requested items
	//
	// Acquires and holds m_item_request_mutex
	void clear_requested_items();

	//
	// Adds an entry into 'm_requested_items' if not present within
	// m_ready_items.
	//
	// Acquires and holds m_item_request_mutex
	void request_item(const item_definition &item);

	// Standard usage, equivalent to passing select_recipes_only = false
	bool process_requested_items();

	// Looks at every item in m_requested_items and tries to populate
	// them into m_ready_items. any items that are not able to be prepared
	// will be within m_unreachable_items.
	//
	// After this call has finished, the user may expect to get results from
	// can_fetch_item(); any entries in m_ready_items will yield 'true',
	// while any entries in m_unreachable_items will yield 'false.'
	// Similarly, fetch_item() can be used to retrieve entries.
	//
	// If select_recipes_only is true, then we will automatically
	// not populate m_ready_items, but instead only populate the
	// m_selected_recipes map.
	//
	// If source_item is non-null, then we will automatically assume
	// this item exists and not have to make the dependency ready.
	//
	// Acquires and holds m_item_request_mutex
	//
	// Returns true if all items were fully available, false otherwise
	bool process_requested_items(bool select_recipes_only, std::optional<item_definition> source_item);

	//
	// Looks at m_ready_items. If the item is present, will return the contained
	// prepared item. Otherwise, throws an exception.
	//
	// Acquires and holds m_item_request_mutex
	std::shared_ptr<prepared_item> fetch_item(const item_definition &item);

	//
	// Looks at m_ready_items. If the item is present, will return true,
	// otherwise, return false.
	//
	// Acquires and holds m_item_request_mutex
	bool can_fetch_item(const item_definition &item);

	//
	// Looks at m_selected_recipes. If the item is present, will return the contained
	// prepared item. Otherwise, throws an exception.
	//
	// Acquires and holds m_item_request_mutex
	std::shared_ptr<recipe> fetch_selected_recipe(const item_definition &item);

	//
	// Looks at m_selected_recipes. If the item is present, will return true,
	// otherwise, return false.
	//
	// Acquires and holds m_item_request_mutex
	bool can_fetch_selected_recipe(const item_definition &item);

	//
	// Slicing methods - delegate to m_slicer
	//
	void request_slice(std::shared_ptr<prepared_item> &to_slice, uint64_t offset, const item_definition &slice)
	{
		m_slicer.request_slice(to_slice, offset, slice);
	}

	// Fetches items from slice store and decrements count
	std::shared_ptr<prepared_item> fetch_slice(const item_definition &item) { return m_slicer.fetch_slice(item); }
	void resume_slicing() { m_slicer.resume_slicing(); }
	void pause_slicing() { m_slicer.pause_slicing(); }
	void cancel_slicing() { m_slicer.cancel_slicing(); }

	void write_item(io::writer &writer, const item_definition &item);

	private:
	bool make_dependency_ready(
		const item_definition &item,
		bool select_recipes_only,
		std::optional<item_definition> source_item,
		std::set<item_definition> &already_using);

	std::atomic<bool> m_ready_for_requests{false};

	std::set<item_definition> m_unreachable_items{};
	std::map<item_definition, std::shared_ptr<prepared_item>> m_ready_items{};
	std::map<item_definition, std::shared_ptr<recipe>> m_selected_recipes{};
	std::set<item_definition> m_requested_items;
	std::mutex m_item_request_mutex;

	slicer m_slicer;

	std::shared_ptr<pantry> m_pantry     = std::make_shared<pantry>();
	std::shared_ptr<cookbook> m_cookbook = std::make_shared<cookbook>();

	std::vector<std::shared_ptr<pantry>> m_all_pantries{m_pantry};
	std::vector<std::shared_ptr<cookbook>> m_all_cookbooks{m_cookbook};
};
} // namespace archive_diff::diffs::core