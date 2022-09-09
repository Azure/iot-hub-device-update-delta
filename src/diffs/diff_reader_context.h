/**
 * @file diff_reader_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "blob_cache.h"
#include "recipe_host.h"

#include "reader.h"
#include "wrapped_reader_sequential_reader.h"

#include <gsl/span>
#include <stack>

namespace diffs
{
class diff_reader_context : public io_utility::wrapped_reader_sequential_reader
{
	public:
	diff_reader_context(io_utility::reader *reader) : wrapped_reader_sequential_reader(reader) {}

	void set_recipe_host(recipe_host *recipe_host) { m_recipe_host = recipe_host; }
	recipe_host *get_recipe_host() const { return m_recipe_host; }

	uint64_t m_inline_asset_total{};
	uint64_t m_remainder_chunk_total{};
	uint64_t m_chunk_table_total{};

	std::stack<uint64_t> m_chunk_table_total_stack;

	std::string get_recipe_type_name(uint32_t index) const;
	void set_recipe_type_name(uint32_t index, const std::string name);

	blob_definition m_current_item_blobdef{};

	private:
	recipe_host *m_recipe_host{};
	std::map<uint32_t, std::string> m_recipe_type_name_map;
};
} // namespace diffs