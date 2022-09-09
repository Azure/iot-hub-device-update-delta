/**
 * @file archive.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <gsl/span>

#include "recipe_host.h"
#include "archive_item.h"

namespace diffs
{
class archive : public recipe_host
{
	public:
	archive() : recipe_host() {}

	const std::vector<diffs::archive_item> &get_chunks() const;

	archive_item *add_chunk(uint64_t length, hash_type hash_type, const char *hash_value, uint64_t hash_value_length);

	archive_item *add_chunk(
		uint64_t offset, uint64_t length, hash_type hash_type, const char *hash_value, uint64_t hash_value_length);

	protected:
	std::vector<archive_item> m_chunks;
};
} // namespace diffs