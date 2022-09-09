/**
 * @file archive_item.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <cstring>

#include "blob.h"
#include "blob_cache.h"

#include "recipe.h"
#include "recipe_host.h"

#include "apply_context.h"

namespace diffs
{
class diff;
class recipe;

class archive_item
{
	public:
	archive_item() = default;
	archive_item(uint64_t offset);
	archive_item(archive_item &&) noexcept = default;
	~archive_item()                        = default;

	archive_item(
		uint64_t offset,
		uint64_t length,
		archive_item_type type,
		hash_type hash_type,
		const char *hash_value,
		size_t hash_value_length) :
		m_offset(offset),
		m_type(type), m_length(length)
	{
		m_hash.m_hash_type = hash_type;
		m_hash.m_hash_data.resize(hash_value_length);
		memcpy(m_hash.m_hash_data.data(), hash_value, hash_value_length);
	}

	void read(diff_reader_context &context, bool in_chunk_table);
	void write(diff_writer_context &context, bool in_chunk_table);

	void apply(diffs::apply_context &context) const;
	void prep_blob_cache(diffs::apply_context &context) const;

	std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;

	uint64_t get_offset() const { return m_offset; }
	uint64_t get_length() const { return m_length; }
	uint64_t get_inline_asset_byte_count() const;

	archive_item_type get_type() const { return m_type; }

	bool has_recipe() const { return m_recipe.get() != nullptr; }
	const recipe *get_recipe() const { return m_recipe.get(); }

	const hash &get_hash() const { return m_hash; }

	const blob_definition get_blobdef() const
	{
		blob_definition blobdef{};
		blobdef.m_hashes.emplace_back(m_hash);
		blobdef.m_length = m_length;
		return blobdef;
	}

	recipe *create_recipe(const recipe_host *recipe_host, const char *recipe_type_name);

	private:
	uint64_t m_offset{};
	archive_item_type m_type{};
	uint64_t m_length{};
	hash m_hash;
	std::unique_ptr<recipe> m_recipe{};
};
} // namespace diffs
