/**
 * @file reader_impl.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "reader_impl.h"

namespace archive_diff::io::sequential
{
void reader_impl::skip(uint64_t to_skip)
{
	const size_t c_read_buffer_size = 32 * 1204;
	std::vector<char> read_buffer;

	auto remaining = to_skip;

	size_t to_reserve = static_cast<size_t>(std::min<uint64_t>(remaining, c_read_buffer_size));

	read_buffer.reserve(to_reserve);

	while (remaining)
	{
		size_t to_read = std::min<size_t>(static_cast<size_t>(remaining), c_read_buffer_size);

		auto actual_read = raw_read_some(std::span<char>{read_buffer.data(), to_read});
		remaining -= actual_read;
	}

	m_read_offset += to_skip;
}

} // namespace archive_diff::io::sequential