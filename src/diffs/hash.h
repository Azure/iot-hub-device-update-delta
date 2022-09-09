/**
 * @file hash.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "user_exception.h"
#include "hash_utility.h"
#include "reader.h"

namespace diffs
{
enum class hash_type : uint32_t
{
	Invalid = 0,
	Md5     = 32771,
	Sha256  = 32780,
};

class diff_reader_context;
class diff_writer_context;

// TODO: Sync up with hash_utility better and make this into a class
struct hash
{
	hash() = default;

	hash(hash_type type, const std::vector<char> data) : m_hash_type(type), m_hash_data(data) {}

	hash(hash_type hash_type, io_utility::reader &reader);

	static void verify_hashes_match(const hash &actual, const hash &expected)
	{
		verify_hashes_match(
			std::string_view(actual.m_hash_data.data(), actual.m_hash_data.size()),
			std::string_view(expected.m_hash_data.data(), expected.m_hash_data.size()));
	}
	static void verify_hashes_match(std::string_view actual, std::string_view expected);
	static void verify_data_against_hash(std::string_view data, const diffs::hash &hash);

	hash_type m_hash_type{hash_type::Invalid};
	std::vector<char> m_hash_data;

	void read(diffs::diff_reader_context &context);
	void write(diffs::diff_writer_context &context);
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

	static size_t get_byte_count_for_hash_type(diffs::hash_type hash_type)
	{
		switch (hash_type)
		{
		case hash_type::Md5:
			return 20;
		case hash_type::Sha256:
			return 32;
		default:
			std::string msg = "diffs::hash::get_byte_count_for_hash_type(): Unexpected hash type: "
			                + std::to_string(static_cast<int>(hash_type));
			throw error_utility::user_exception(error_utility::error_code::diff_bad_hash_type, msg);
		}
	}

	static hash_utility::algorithm hash_type_to_algorithm(hash_type hash_type)
	{
		switch (hash_type)
		{
		case hash_type::Md5:
			return hash_utility::algorithm::MD5;
		case hash_type::Sha256:
			return hash_utility::algorithm::SHA256;
		default: {
			std::string msg = "diffs::hash::hash_type_to_algorithm(): Unexpected hash type: "
			                + std::to_string(static_cast<int>(hash_type));
			throw error_utility::user_exception(error_utility::error_code::diff_bad_hash_type, msg);
		}
		}
	}

	static std::string get_hex_string(std::string_view data)
	{
		const char *hex_digits = "0123456789ABCDEF";
		std::string hex_string;

		for (size_t i = 0; i < data.size(); i++)
		{
			unsigned char b = data[i];
			hex_string += hex_digits[b / 16];
			hex_string += hex_digits[b % 16];
		}
		return hex_string;
	}
};
} // namespace diffs