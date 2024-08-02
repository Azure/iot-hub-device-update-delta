/**
 * @file pantry.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "item_definition.h"
#include "prepared_item.h"
#include "prepared_item_lookup.h"

namespace archive_diff::diffs::core
{
class pantry
{
	public:
	bool find(const item_definition &item, std::shared_ptr<prepared_item> *result) const
	{
		return m_lookup.find(item, result);
	}

	void add(std::shared_ptr<prepared_item> &prepared)
	{
		auto &result = prepared->get_item_definition();
		m_lookup.add(result, prepared);
		m_items.insert(std::pair{result, prepared});
	}

	bool find(const std::string &name, std::shared_ptr<prepared_item> *result) { return m_lookup.find(name, result); }

	using item_definition_store = std::map<item_definition, std::shared_ptr<prepared_item>>;

	const item_definition_store get_items() const { return m_items; }

	protected:
	// Content that is available forever. This includes things from user input or inline asset+s
	prepared_item_lookup m_lookup;
	item_definition_store m_items;
};
} // namespace archive_diff::diffs::core