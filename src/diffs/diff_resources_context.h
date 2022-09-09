/**
 * @file diff_resources_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <gsl/span>

#include "reader.h"
#include "sequential_reader.h"

#include "zlib_decompression_reader.h"

namespace diffs
{
class diff;

class diff_resources_context
{
	public:
	diff_resources_context(diff_resources_context *parent_context);

	// Diff is maintained while this context is around
	diff_resources_context(
		io_utility::unique_reader &&inline_assets_reader, io_utility::unique_sequential_reader &&remainder_reader);

	diff_resources_context(diff_resources_context &&) noexcept = default;

	diff_resources_context from_diff(diff *diff, io_utility::reader *reader);

	virtual ~diff_resources_context() = default;

	std::unique_ptr<io_utility::reader> get_inline_asset_reader(uint64_t offset, uint64_t length);
	io_utility::sequential_reader *get_remainder_reader() { return m_remainder_reader; }

	private:
	io_utility::unique_reader m_inline_assets_reader_storage;
	io_utility::unique_sequential_reader m_remainder_reader_storage;

	io_utility::reader *m_inline_assets_reader{};
	io_utility::sequential_reader *m_remainder_reader{};
};
} // namespace diffs
