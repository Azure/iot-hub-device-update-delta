/**
 * @file hash.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string>
#include <cstring>

#include <errors/user_exception.h>

#include "hash.h"
#include "hasher.h"
#include "hexstring_convert.h"

namespace archive_diff::hashing
{
hash::hash(algorithm algorithm, io::reader &reader) : m_algorithm(algorithm)
{
	hashing::hasher hasher{hashing::algorithm::sha256};

	auto remaining = reader.size();
	uint64_t offset{};

	const size_t MAX_READ_CHUNK = static_cast<size_t>(64 * 1024);
	std::vector<char> read_buffer;

	while (remaining)
	{
		auto to_read = std::min<size_t>(MAX_READ_CHUNK, remaining);
		read_buffer.reserve(to_read);

		reader.read(offset, std::span<char>{read_buffer.data(), to_read});

		hasher.hash_data(std::string_view{read_buffer.data(), to_read});

		offset += to_read;
		remaining -= to_read;
	}

	m_hash_data = hasher.get_hash_binary();
}

void hash::read(io::sequential::reader& reader)
{
	reader.read(&m_algorithm);
	size_t hash_data_bytes = get_byte_count_for_algorithm(m_algorithm);
	m_hash_data.resize(hash_data_bytes);

	memset(m_hash_data.data(), 0, hash_data_bytes);
	reader.read(std::span{m_hash_data.data(), hash_data_bytes});
}

void hash::write(io::sequential::writer& writer) const
{
	writer.write_value(m_algorithm);
	writer.write(std::string_view{m_hash_data.data(), m_hash_data.size()});
}

std::string hash::get_type_string() const
{
	switch (m_algorithm)
	{
	case algorithm::md5:
		return std::string("Md5");
	case algorithm::sha256:
		return std::string("Sha256");
	default:
		std::string msg =
			"hash::get_type_string(): Unexpected hash type: " + std::to_string(static_cast<int>(m_algorithm));
		throw errors::user_exception(errors::error_code::diff_bad_hash_type, msg);
	}
}

std::string hash::get_data_string() const
{
	return hashing::data_to_hexstring(m_hash_data.data(), m_hash_data.size());
}

void hash::verify_hashes_match(std::string_view actual, std::string_view expected)
{
#ifndef DISABLE_ALL_HASHING
	if (actual.size() != expected.size())
	{
		std::string msg = "hash::verify_hashes_match(): Hash size mismatch. Actual: " + std::to_string(actual.size())
		                + ", Expected: " + std::to_string(expected.size());
		throw errors::user_exception(errors::error_code::diff_verify_hash_failure, msg);
	}

	if (memcmp(actual.data(), expected.data(), expected.size()))
	{
		std::string msg = "Actual and expected buffers mismatch.";
		msg += " Actual: " + data_to_hexstring(actual.data(), actual.size());
		msg += " Expected: " + data_to_hexstring(expected.data(), expected.size());
		throw errors::user_exception(errors::error_code::diff_verify_hash_failure, msg);
	}
#endif
}

void hash::verify_data_against_hash(std::string_view data, const hash &hash)
{
	hashing::hasher hasher(hashing::algorithm::sha256);
	hasher.hash_data(data.data(), data.size());
	auto calculated_hash = hasher.get_hash_binary();

	std::string_view actual(reinterpret_cast<char *>(calculated_hash.data()), calculated_hash.size());
	std::string_view expected(hash.m_hash_data.data(), hash.m_hash_data.size());
	verify_hashes_match(actual, expected);
}

} // namespace archive_diff::hashing