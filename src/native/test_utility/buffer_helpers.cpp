/**
 * @file buffer_helpers.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "buffer_helpers.h"

#include <hashing/hasher.h>

archive_diff::diffs::core::item_definition create_definition_from_data(std::string_view data)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);

	hasher.hash_data(data.data(), data.size());
	auto hash = hasher.get_hash();

	return archive_diff::diffs::core::item_definition(data.size()).with_hash(hash);
}

archive_diff::diffs::core::item_definition create_definition_from_vector_using_capacity(std::vector<char> &data)
{
	return create_definition_from_data(std::string_view{data.data(), data.capacity()});
}

archive_diff::diffs::core::item_definition create_definition_from_span(const std::span<char> data)
{
	return create_definition_from_data(std::string_view{data.data(), data.size()});
}

void modify_vector(
	std::vector<char> &data, size_t new_size, size_t set_to_four_ratio, size_t bitmask_ratio, size_t xor_ratio)
{
	data.resize(new_size);

	for (size_t i = 0; i < data.size(); i++)
	{
		if (i % set_to_four_ratio == 0)
		{
			data[i] = 4;
		}

		if (i % bitmask_ratio == 0)
		{
			data[i] |= 33;
		}

		if (i % xor_ratio == 0)
		{
			data[i] ^= 0xFEF1F0F3;
		}
	}
}