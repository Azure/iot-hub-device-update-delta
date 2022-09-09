/**
 * @file diff_viewer_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diff_viewer_context.h"

#include "diff.h"

diffs::diff_viewer_context::diff_viewer_context(diffs::diff *diff, io_utility::reader *diff_reader) :
	m_diff_reader(diff_reader)
{
	m_inline_assets_reader_storage = diff->make_inline_assets_reader(diff_reader);
	m_remainder_reader_storage     = diff->make_remainder_reader(diff_reader);

	m_inline_assets_reader = m_inline_assets_reader_storage.get();
	m_remainder_reader     = m_remainder_reader_storage.get();
}

diffs::diff_viewer_context::diff_viewer_context(diff_viewer_context *parent_context) :
	m_diff_reader(parent_context->m_diff_reader), m_inline_assets_reader(parent_context->m_inline_assets_reader),
	m_remainder_reader(parent_context->m_remainder_reader)
{}

std::unique_ptr<io_utility::reader> diffs::diff_viewer_context::get_inline_asset_reader(
	uint64_t offset, uint64_t length)
{
	return std::make_unique<io_utility::child_reader>(m_inline_assets_reader, offset, length);
}
