/**
 * @file writer_impl.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "writer_impl.h"

namespace archive_diff::io::sequential
{
void writer_impl::write(std::string_view buffer)
{
	write_impl(buffer);
	m_offset += buffer.size();
}

uint64_t writer_impl::tellp() { return m_offset; }
} // namespace archive_diff::io::sequential