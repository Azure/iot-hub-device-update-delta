/**
 * @file archive.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "archive.h"

const std::vector<diffs::archive_item> &diffs::archive::get_chunks() const { return m_chunks; }

diffs::archive_item *diffs::archive::add_chunk(
	uint64_t length, hash_type hash_type, const char *hash_value, uint64_t hash_value_length)
{
	uint64_t offset{};
	if (m_chunks.size() != 0)
	{
		auto &last_chunk = m_chunks.back();

		offset = last_chunk.get_offset() + last_chunk.get_length();
	}

	return add_chunk(offset, length, hash_type, hash_value, hash_value_length);
}

diffs::archive_item *diffs::archive::add_chunk(
	uint64_t offset, uint64_t length, hash_type hash_type, const char *hash_value, uint64_t hash_value_length)
{
	m_chunks.emplace_back(archive_item{
		offset, length, archive_item_type::chunk, hash_type, hash_value, static_cast<size_t>(hash_value_length)});

	return &m_chunks.back();
}
