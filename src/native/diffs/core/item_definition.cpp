/**
 * @file item_definition.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "item_definition.h"

#include <io/sequential/basic_reader_wrapper.h>

#include <errors/user_exception.h>

namespace archive_diff::diffs::core
{
bool item_definition::operator<(const item_definition &rhs) const
{
	if (m_length != rhs.m_length)
	{
		return m_length < rhs.m_length;
	}

	if (m_hashes.size() != rhs.m_hashes.size())
	{
		return m_hashes.size() < rhs.m_hashes.size();
	}

	for (auto algorithm : hashing::all_algorithms)
	{
		bool left_has_hash  = m_hashes.count(algorithm) > 0;
		bool right_has_hash = rhs.m_hashes.count(algorithm) > 0;

		if (!left_has_hash && !right_has_hash)
		{
			continue;
		}

		if (!right_has_hash) // hash appears in left only
		{
			return false;
		}
		else if (!left_has_hash) // hash appears in right only
		{
			return true;
		}

		auto left_hash  = &m_hashes.find(algorithm)->second;
		auto right_hash = &rhs.m_hashes.find(algorithm)->second;

		// ok, both have this hash, compare the bytes
		if (left_hash->m_hash_data.size() != right_hash->m_hash_data.size())
		{
			throw errors::user_exception(errors::error_code::item_definition_hash_size_mismatch);
		}

		auto cmp =
			std::memcmp(left_hash->m_hash_data.data(), right_hash->m_hash_data.data(), right_hash->m_hash_data.size());

		if (cmp != 0)
		{
			return cmp < 0;
		}
	}

	return false;
}

bool item_definition::operator==(const item_definition &rhs) const
{
	if (m_length != rhs.m_length)
	{
		return false;
	}

	if (m_hashes.size() != rhs.m_hashes.size())
	{
		return false;
	}

	for (auto algorithm : hashing::all_algorithms)
	{
		bool left_has_hash  = m_hashes.count(algorithm) > 0;
		bool right_has_hash = rhs.m_hashes.count(algorithm) > 0;

		if (!left_has_hash && !right_has_hash)
		{
			continue;
		}

		if (!right_has_hash || !left_has_hash)
		{
			return false;
		}

		auto left_hash  = &m_hashes.find(algorithm)->second;
		auto right_hash = &rhs.m_hashes.find(algorithm)->second;

		// ok, both have this hash, compare the bytes
		if (left_hash->m_hash_data.size() != right_hash->m_hash_data.size())
		{
			throw errors::user_exception(errors::error_code::item_definition_hash_size_mismatch);
		}

		auto cmp =
			std::memcmp(left_hash->m_hash_data.data(), right_hash->m_hash_data.data(), right_hash->m_hash_data.size());

		if (cmp != 0)
		{
			return false;
		}
	}

	return true;
}

[[nodiscard]] item_definition item_definition::with_name(const std::string &name) const
{
	item_definition new_item(*this);
	new_item.m_names.insert(name);
	return new_item;
}

[[nodiscard]] item_definition item_definition::with_hash(const hashing::hash &hash) const
{
	auto algorithm = hash.m_algorithm;

	if (m_hashes.count(algorithm))
	{
		auto find_itr   = m_hashes.find(algorithm);
		auto &hash_data = find_itr->second.m_hash_data;

		if (hash_data.size() != hash.m_hash_data.size())
		{
			throw errors::user_exception(errors::error_code::item_definition_hash_size_mismatch);
		}

		if (0 != std::memcmp(hash_data.data(), hash.m_hash_data.data(), hash_data.size()))
		{
			throw errors::user_exception(errors::error_code::item_definition_hash_same_type_different_value);
		}

		// don't need to add a duplicate
		return item_definition(*this);
	}

	item_definition new_item(*this);
	new_item.m_hashes.insert(std::pair{hash.m_algorithm, hash});
	return new_item;
}

bool item_definition::has_matching_hash(hashing::hash &hash) const
{
	auto algorithm = hash.m_algorithm;

	if (m_hashes.count(algorithm) == 0)
	{
		return false;
	}

	auto find_itr   = m_hashes.find(algorithm);
	auto &hash_data = find_itr->second.m_hash_data;

	if (hash_data.size() != hash.m_hash_data.size())
	{
		throw errors::user_exception(errors::error_code::item_definition_hash_size_mismatch);
	}

	return (0 == std::memcmp(hash_data.data(), hash.m_hash_data.data(), hash_data.size()));
}

item_definition::match_result item_definition::match(const item_definition &rhs) const
{
	auto result = match_result::uncertain;

	if (m_length != rhs.m_length)
	{
		return match_result::no_match;
	}

	for (auto algorithm : hashing::all_algorithms)
	{
		bool left_has_hash  = m_hashes.count(algorithm) > 0;
		bool right_has_hash = rhs.m_hashes.count(algorithm) > 0;

		if (!left_has_hash && !right_has_hash)
		{
			continue;
		}

		if (!right_has_hash)
		{
			continue;
		}
		else if (!left_has_hash)
		{
			continue;
		}

		auto left_hash  = &m_hashes.find(algorithm)->second;
		auto right_hash = &rhs.m_hashes.find(algorithm)->second;

		// ok, both have this hash, compare the bytes
		if (left_hash->m_hash_data.size() != right_hash->m_hash_data.size())
		{
			throw errors::user_exception(errors::error_code::item_definition_hash_size_mismatch);
		}

		auto cmp =
			std::memcmp(left_hash->m_hash_data.data(), right_hash->m_hash_data.data(), right_hash->m_hash_data.size());

		if (cmp != 0)
		{
			return match_result::no_match;
		}

		result = match_result::match;
	}
	return result;
}

std::string item_definition::to_string() const
{
	std::string str = "{len=" + std::to_string(m_length);

	if (!m_names.empty())
	{
		str += ", names: {";
		auto name_entry = m_names.cbegin();
		while (name_entry != m_names.cend())
		{
			str += *name_entry;

			name_entry++;
			if (name_entry != m_names.cend())
			{
				str += ", ";
			}
		}
		str += "}";
	}

	if (m_hashes.size() != 0)
	{
		str += ", hashes={";
		auto hash_entry = m_hashes.cbegin();
		while (hash_entry != m_hashes.cend())
		{
			auto &hash = hash_entry->second;
			str += "\"";
			str += hash.get_string();
			str += "\"";

			hash_entry++;
			if (hash_entry != m_hashes.cend())
			{
				str += ", ";
			}
		}
	}

	str += "}";

	return str;
}

void item_definition::write(io::sequential::writer &writer, serialization_options options) const
{
	bool include_hashes           = (options & serialization_options::include_hashes) > 0;
	bool include_names            = (options & serialization_options::include_names) > 0;
	bool zero_size_has_no_details = (options & serialization_options::zero_size_has_no_details) > 0;
	bool include_only_sha256_hash = (options & serialization_options::include_only_sha256_hash) > 0;

	writer.write_value(m_length);

	if ((m_length == 0) && zero_size_has_no_details)
	{
		return;
	}

	if (include_hashes)
	{
		if (include_only_sha256_hash)
		{
			auto itr = m_hashes.find(hashing::algorithm::sha256);

			if (itr == m_hashes.cend())
			{
				throw errors::user_exception(errors::error_code::item_definition_no_sha256_hash);
			}

			auto &hash = itr->second;
			hash.write(writer);
		}
		else
		{
			// we really shouldn't expect a lot of hashes!
			writer.write_value(static_cast<uint8_t>(m_hashes.size()));
			for (auto &hash_entry : m_hashes)
			{
				auto &hash = hash_entry.second;
				hash.write(writer);
			}
		}
	}

	if (include_names)
	{
		// TODO: Implement
	}
}

item_definition item_definition::read(io::sequential::reader &reader, serialization_options options)
{
	bool include_hashes           = (options & serialization_options::include_hashes) > 0;
	bool include_names            = (options & serialization_options::include_names) > 0;
	bool zero_size_has_no_details = (options & serialization_options::zero_size_has_no_details) > 0;
	bool include_only_sha256_hash = (options & serialization_options::include_only_sha256_hash) > 0;

	uint64_t length;
	reader.read(&length);

	item_definition item{length};

	if ((length == 0) && zero_size_has_no_details)
	{
		return item;
	}

	if (include_hashes)
	{
		if (include_only_sha256_hash)
		{
			hashing::hash hash;
			hash.read(reader);

			item = item.with_hash(hash);
		}
		else
		{
			uint8_t hash_count;
			reader.read(&hash_count);

			for (uint8_t i = 0; i < hash_count; i++)
			{
				hashing::hash hash;
				hash.read(reader);

				item = item.with_hash(hash);
			}
		}
	}

	if (include_names)
	{
		// TODO: Implement
	}

	return item;
}

item_definition item_definition::read(io::reader &reader, serialization_options options)
{
	io::sequential::basic_reader_wrapper seq(reader);

	return read(seq, options);
}

item_definition::serialization_options operator|(
	const item_definition::serialization_options &lhs, const item_definition::serialization_options &rhs)
{
	return static_cast<item_definition::serialization_options>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

item_definition::serialization_options operator&(
	const item_definition::serialization_options &lhs, const item_definition::serialization_options &rhs)
{
	return static_cast<item_definition::serialization_options>(static_cast<int>(lhs) & static_cast<int>(rhs));
}
} // namespace archive_diff::diffs::core