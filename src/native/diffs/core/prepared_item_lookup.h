/**
 * @file prepared_item_lookup.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include <hashing/hash.h>

#include "item_definition.h"
#include "prepared_item.h"

#include "adu_log.h"

namespace archive_diff::diffs::core
{
class prepared_item_lookup
{
	public:
	bool find(const core::item_definition &item, std::shared_ptr<prepared_item> *result) const
	{
		for (auto &hash : item.get_hashes())
		{
			std::pair<uint64_t, hashing::hash> key{item.size(), hash.second};

			auto find_itr = m_hash_lookup.find(key);
			if (find_itr == m_hash_lookup.cend())
			{
				continue;
			}

			if (result)
			{
				*result = find_itr->second;
			}
			return true;
		}

		for (auto &name : item.get_names())
		{
			if (find(name, result))
			{
				return true;
			}
		}

		return false;
	}

	bool find(const std::string &name, std::shared_ptr<prepared_item> *result) const
	{
		auto find_itr = m_name_lookup.find(name);
		if (find_itr == m_name_lookup.cend())
		{
			return false;
		}

		if (result)
		{
			*result = find_itr->second;
		}
		return true;
	}

	void add(const core::item_definition &item, const std::shared_ptr<prepared_item> &value)
	{
		for (auto &hash : item.get_hashes())
		{
			std::pair<uint64_t, hashing::hash> key{item.size(), hash.second};
			m_hash_lookup.insert(std::pair{key, value});
		}

		for (auto &name : item.get_names())
		{
			m_name_lookup.insert(std::pair{name, value});
		}
	}

	private:
	std::map<std::pair<uint64_t, hashing::hash>, std::shared_ptr<prepared_item>> m_hash_lookup;
	std::map<std::string, std::shared_ptr<prepared_item>> m_name_lookup;
};
} // namespace archive_diff::diffs::core
