/**
 * @file hash.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstring>
#include <cstdint>
#include <vector>
#include <string>

#include <errors/user_exception.h>
#include <io/reader.h>

#include <io/sequential/reader.h>
#include <io/sequential/writer.h>

#include "algorithm.h"

namespace archive_diff::hashing
{
struct hash
{
	hash() : m_algorithm(algorithm::invalid) {}
	hash(algorithm algo, io::reader &reader);

	static hash import_hash_value(algorithm algo, std::string_view hash_value)
	{
		hash new_hash;
		new_hash.m_algorithm = algo;

		auto expected_byte_count = get_byte_count_for_algorithm(algo);

		if (hash_value.size() != expected_byte_count)
		{
			throw std::exception();
		}

		new_hash.m_hash_data.resize(expected_byte_count);
		std::memcpy(new_hash.m_hash_data.data(), hash_value.data(), hash_value.size());

		return new_hash;
	}

	void read(io::sequential::reader& reader);
	void write(io::sequential::writer& writer) const;

	bool operator<(const hash &rhs) const
	{
		if (m_algorithm != rhs.m_algorithm)
		{
			return m_algorithm < rhs.m_algorithm;
		}

		if (m_hash_data.size() != rhs.m_hash_data.size())
		{
			return m_hash_data.size() < rhs.m_hash_data.size();
		}

		return std::memcmp(m_hash_data.data(), rhs.m_hash_data.data(), m_hash_data.size()) < 0;
	}

	static void verify_hashes_match(const hash &actual, const hash &expected)
	{
		verify_hashes_match(
			std::string_view(actual.m_hash_data.data(), actual.m_hash_data.size()),
			std::string_view(expected.m_hash_data.data(), expected.m_hash_data.size()));
	}
	static void verify_hashes_match(std::string_view actual, std::string_view expected);
	static void verify_data_against_hash(std::string_view data, const hash &hash);

	algorithm m_algorithm{algorithm::invalid};
	std::vector<char> m_hash_data;

	std::string get_type_string() const;
	std::string get_data_string() const;
	std::string get_string() const
	{
		std::string value = get_data_string();
		value += "(";
		value += get_type_string();
		value += ")";

		return value;
	}
};
} // namespace archive_diff::hashing