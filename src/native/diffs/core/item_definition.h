/**
 * @file item_definition.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include <hashing/hash.h>

#include <json/json.h>

namespace archive_diff::diffs::core
{
class item_definition
{
	public:
	struct name_based_key
	{
		std::string m_name;
		uint64_t m_length;

		bool operator<(const name_based_key &rhs) const
		{
			if (m_length != rhs.m_length)
			{
				return m_length < rhs.m_length;
			}

			if (m_name != rhs.m_name)
			{
				return m_name < rhs.m_name;
			}

			return false;
		}
	};

	public:
	item_definition() : item_definition(0){};
	item_definition(uint64_t length) { m_length = length; }

	bool operator<(const item_definition &rhs) const;
	bool operator!=(const item_definition &rhs) const { return !(*this == rhs); }
	bool operator==(const item_definition &rhs) const;

	[[nodiscard]] item_definition with_name(const std::string &name) const;
	[[nodiscard]] item_definition with_hash(const hashing::hash &hash) const;

	using item_names_set  = std::set<std::string>;
	using item_hashes_map = std::map<hashing::algorithm, hashing::hash>;

	const item_hashes_map &get_hashes() const { return m_hashes; }
	const item_names_set &get_names() const { return m_names; }

	bool has_matching_hash(hashing::hash &hash) const;
	bool has_matching_name(const std::string &name) const { return m_names.count(name) > 0; }
	bool has_hash_for_alg(hashing::algorithm algo) const { return m_hashes.count(algo) > 0; }
	bool has_any_hashes() const { return !m_hashes.empty(); }

	uint64_t size() const { return m_length; }

	enum class match_result
	{
		uncertain = -1,
		no_match  = 0,
		match     = 1,
	};
	match_result match(const item_definition &rhs) const;
	bool equals(const item_definition &other) const { return match(other) == match_result::match; }

	std::string to_string() const;

	Json::Value to_json() const;

	enum serialization_options : int
	{
		include_hashes           = 0x1,
		include_names            = 0x2,
		zero_size_has_no_details = 0x4,
		include_only_sha256_hash = 0x8,
		standard                 = include_hashes | include_names | zero_size_has_no_details | include_only_sha256_hash,
		legacy                   = include_hashes | zero_size_has_no_details | include_only_sha256_hash,
	};
	void write(io::sequential::writer &writer, serialization_options options) const;

	static item_definition read(io::sequential::reader &reader, serialization_options options);
	static item_definition read(io::reader &reader, serialization_options options);

	private:
	uint64_t m_length{};
	item_hashes_map m_hashes{};
	item_names_set m_names{};
};

item_definition::serialization_options operator|(
	const item_definition::serialization_options &lhs, const item_definition::serialization_options &rhs);
item_definition::serialization_options operator&(
	const item_definition::serialization_options &lhs, const item_definition::serialization_options &rhs);
} // namespace archive_diff::diffs::core