/**
 * @file source_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"

namespace diffs
{
class source_context
{
	public:
	source_context(source_context *parent_context) : m_source_reader(parent_context->m_source_reader) {}

	source_context(io_utility::unique_reader &&reader) :
		m_source_reader_storage(std::move(reader)), m_source_reader(m_source_reader_storage.get())
	{}

	std::unique_ptr<io_utility::reader> get_source_reader(uint64_t source_offset, uint64_t source_length);
	io_utility::reader *get_source_reader() { return m_source_reader; }

	private:
	io_utility::unique_reader m_source_reader_storage;
	io_utility::reader *m_source_reader{};
};
} // namespace diffs