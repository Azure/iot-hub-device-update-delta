/**
 * @file hash.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string>
#include <cstring>

#include "user_exception.h"

#include "hash.h"

#include "hash_utility.h"

#include "diff_reader_context.h"
#include "diff_writer_context.h"

diffs::hash::hash(hash_type hash_type, io_utility::reader &reader) : m_hash_type(hash_type)
{
	auto algorithm = hash_type_to_algorithm(hash_type);
	hash_utility::hasher hasher{hash_utility::algorithm::SHA256};

	auto remaining = reader.size();
	uint64_t offset{};

	const size_t MAX_READ_CHUNK = static_cast<size_t>(64 * 1024);
	std::vector<char> read_buffer;

	while (remaining)
	{
		auto to_read = std::min<size_t>(MAX_READ_CHUNK, remaining);
		read_buffer.reserve(to_read);

		reader.read(offset, gsl::span<char>{read_buffer.data(), to_read});

		hasher.hash_data(std::string_view{read_buffer.data(), to_read});

		offset += to_read;
		remaining -= to_read;
	}

	m_hash_data = hasher.get_hash_binary();
}

void diffs::hash::read(diffs::diff_reader_context &context)
{
	context.read(&m_hash_type);
	size_t hash_data_bytes = get_byte_count_for_hash_type(m_hash_type);
	m_hash_data.resize(hash_data_bytes);

	memset(m_hash_data.data(), 0, hash_data_bytes);
	context.read(gsl::span{m_hash_data.data(), hash_data_bytes});
}

void diffs::hash::write(diffs::diff_writer_context &context)
{
	context.write(m_hash_type);
	context.write(std::string_view{m_hash_data.data(), m_hash_data.size()});
}

std::string diffs::hash::get_type_string() const
{
	switch (m_hash_type)
	{
	case hash_type::Md5:
		return std::string("Md5");
	case hash_type::Sha256:
		return std::string("Sha256");
	default:
		std::string msg =
			"diffs::hash::get_type_string(): Unexpected hash type: " + std::to_string(static_cast<int>(m_hash_type));
		throw error_utility::user_exception(error_utility::error_code::diff_bad_hash_type, msg);
	}
}

std::string diffs::hash::get_data_string() const
{
	return get_hex_string(std::string_view(m_hash_data.data(), m_hash_data.size()));
}

void diffs::hash::verify_hashes_match(std::string_view actual, std::string_view expected)
{
#ifndef DISABLE_ALL_HASHING
	if (actual.size() != expected.size())
	{
		std::string msg = "diffs::hash::verify_hashes_match(): Hash size mismatch. Actual: "
		                + std::to_string(actual.size()) + ", Expected: " + std::to_string(expected.size());
		throw error_utility::user_exception(error_utility::error_code::diff_verify_hash_failure, msg);
	}

	if (memcmp(actual.data(), expected.data(), expected.size()))
	{
		std::string msg = "Actual and expected buffers mismatch.";
		msg += " Actual: " + diffs::hash::get_hex_string(std::string_view(actual.data(), actual.size()));
		msg += " Expected: " + diffs::hash::get_hex_string(std::string_view(expected.data(), expected.size()));
		throw error_utility::user_exception(error_utility::error_code::diff_verify_hash_failure, msg);
	}
#endif
}

void diffs::hash::verify_data_against_hash(std::string_view data, const diffs::hash &hash)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	hasher.hash_data(data.data(), data.size());
	auto calculated_hash = hasher.get_hash_binary();

	std::string_view actual(reinterpret_cast<char *>(calculated_hash.data()), calculated_hash.size());
	std::string_view expected(hash.m_hash_data.data(), hash.m_hash_data.size());
	verify_hashes_match(actual, expected);
}
