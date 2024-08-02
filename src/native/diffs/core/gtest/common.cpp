/**
 * @file common.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "common.h"

#include <hashing/hasher.h>

item_definition create_definition_from_data(std::vector<char> &data)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
	hasher.hash_data(std::string_view{data.data(), data.capacity()});
	auto hash = hasher.get_hash();

	return item_definition{data.size()}.with_hash(hash);
}
