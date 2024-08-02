/**
 * @file recipe_lookup.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include <hashing/hash.h>

#include "item_definition.h"
#include "recipe_set.h"

namespace archive_diff::diffs::core
{
class recipe_lookup
{
	public:
	bool find(const core::item_definition &item, const recipe_set **result)
	{
		for (auto &hash : item.get_hashes())
		{
			std::pair<uint64_t, hashing::hash> key{item.size(), hash.second};

			auto find_itr = m_map.find(key);
			if (find_itr == m_map.cend())
			{
				continue;
			}

			*result = &find_itr->second;
			return true;
		}
		return false;
	}

	void add(const core::item_definition &item, const std::shared_ptr<recipe> &recipe)
	{
		for (auto &hash : item.get_hashes())
		{
			std::pair<uint64_t, hashing::hash> key{item.size(), hash.second};

			if (m_map.count(key) == 0)
			{
				m_map.insert(std::pair{key, recipe_set{}});
			}
			m_map.at(key).insert(recipe);
		}
	}

	private:
	std::map<std::pair<uint64_t, hashing::hash>, recipe_set> m_map;
};
} // namespace archive_diff::diffs::core
