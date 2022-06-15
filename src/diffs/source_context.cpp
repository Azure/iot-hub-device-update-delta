/**
 * @file source_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "source_context.h"

#include "child_reader.h"

io_utility::unique_reader diffs::source_context::get_source_reader(uint64_t source_offset, uint64_t source_length)
{
	return std::make_unique<io_utility::child_reader>(m_source_reader, source_offset, source_length);
}