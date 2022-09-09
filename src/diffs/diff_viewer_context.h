/**
 * @file diff_viewer_context.h
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

class diff_viewer_context
{
	public:
	diff_viewer_context(diff_viewer_context *parent_context);
	diff_viewer_context(diffs::diff *diff, io_utility::reader *diff_reader);

	std::unique_ptr<io_utility::reader> get_inline_asset_reader(uint64_t offset, uint64_t length);
	io_utility::sequential_reader *get_remainder_reader() { return m_remainder_reader; }

	io_utility::reader *get_diff_reader() { return m_diff_reader; }

	private:
	io_utility::reader *m_diff_reader{};

	std::unique_ptr<io_utility::reader> m_inline_assets_reader_storage;
	io_utility::reader *m_inline_assets_reader{};

	std::unique_ptr<io_utility::sequential_reader> m_remainder_reader_storage;
	io_utility::sequential_reader *m_remainder_reader{};
};
} // namespace diffs