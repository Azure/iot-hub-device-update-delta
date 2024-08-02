/**
 * @file item_definition_helpers.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "item_definition_helpers.h"

#include <hashing/hasher.h>

namespace archive_diff::diffs::core
{
item_definition create_definition_from_span(const std::span<char> data)
{
	return create_definition_from_string_view(std::string_view{data.data(), data.size()});
}

item_definition create_definition_from_string_view(std::string_view data)
{
	hashing::hasher hasher(hashing::algorithm::sha256);

	hasher.hash_data(data.data(), data.size());
	auto hash = hasher.get_hash();

	return diffs::core::item_definition(data.size()).with_hash(hash);
}

item_definition create_definition_from_vector_using_capacity(std::vector<char> &data)
{
	return create_definition_from_string_view(std::string_view{data.data(), data.capacity()});
}

item_definition create_definition_from_vector_using_size(std::vector<char> &data)
{
	return create_definition_from_string_view(std::string_view{data.data(), data.size()});
}

item_definition create_definition_from_reader(io::reader &reader)
{
	hashing::hasher hasher(hashing::algorithm::sha256);

	const size_t block_size = 8 * 1024;

	char data[block_size];

	auto remaining = reader.size();
	uint64_t offset{0};

	while (remaining)
	{
		auto to_read = static_cast<size_t>(std::min<uint64_t>(remaining, sizeof(data)));
		reader.read(offset, std::span{data, to_read});
		hasher.hash_data(std::string_view{data, to_read});

		remaining -= to_read;
		offset += to_read;
	}

	auto hash = hasher.get_hash();
	return diffs::core::item_definition(reader.size()).with_hash(hash);
}

item_definition create_definition_from_sequential_reader(io::sequential::reader *reader)
{
	hashing::hasher hasher(hashing::algorithm::sha256);

	const size_t block_size = 8 * 1024;

	char data[block_size];

	auto remaining = reader->size();

	while (remaining)
	{
		auto to_read = static_cast<size_t>(std::min<uint64_t>(remaining, sizeof(data)));
		reader->read(std::span{data, to_read});
		hasher.hash_data(std::string_view{data, to_read});

		remaining -= to_read;
	}

	auto hash = hasher.get_hash();
	return diffs::core::item_definition(reader->size()).with_hash(hash);
}
} // namespace archive_diff::diffs::core