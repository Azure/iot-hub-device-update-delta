/**
 * @file diff_reader_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "blob_cache.h"

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

	uint64_t m_inline_asset_total{};
	uint64_t m_remainder_chunk_total{};
	uint64_t m_chunk_table_total{};

	std::stack<uint64_t> m_chunk_table_total_stack;

	blob_definition m_current_item_blobdef{};
};
} // namespace diffs