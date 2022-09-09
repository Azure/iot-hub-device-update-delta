/**
 * @file diff_resources_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diff_resources_context.h"

#include "diff.h"

diffs::diff_resources_context::diff_resources_context(diff_resources_context *parent_context) :
	m_inline_assets_reader(parent_context->m_inline_assets_reader),
	m_remainder_reader(parent_context->m_remainder_reader)
{}

diffs::diff_resources_context::diff_resources_context(
	io_utility::unique_reader &&inline_assets_reader, io_utility::unique_sequential_reader &&remainder_reader)
{
	m_inline_assets_reader_storage = std::move(inline_assets_reader);
	m_remainder_reader_storage     = std::move(remainder_reader);

	m_inline_assets_reader = m_inline_assets_reader_storage.get();
	m_remainder_reader     = m_remainder_reader_storage.get();
}

std::unique_ptr<io_utility::reader> diffs::diff_resources_context::get_inline_asset_reader(
	uint64_t offset, uint64_t length)
{
	return std::make_unique<io_utility::child_reader>(m_inline_assets_reader, offset, length);
}

diffs::diff_resources_context diffs::diff_resources_context::from_diff(diff *diff, io_utility::reader *reader)
{
	return diff_resources_context{diff->make_inline_assets_reader(reader), diff->make_remainder_reader(reader)};
}