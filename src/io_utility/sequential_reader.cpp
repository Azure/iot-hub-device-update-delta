/**
 * @file sequential_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <vector>

#include "sequential_reader.h"

#include "user_exception.h"

size_t io_utility::sequential_reader::read_some(uint64_t offset, gsl::span<char> buffer)
{
	if (m_read_offset > offset)
	{
		std::string msg = "Trying to read in previous section of sequential reader. Offset: " + std::to_string(offset)
		                + " m_read_offset: " + std::to_string(m_read_offset);
		throw error_utility::user_exception(error_utility::error_code::io_sequential_reader_bad_offset, msg);
	}

	if (m_read_offset < offset)
	{
		skip(offset - m_read_offset);
	}

	return read_some(buffer);
}

void io_utility::sequential_reader::skip(uint64_t to_skip)
{
	const size_t c_read_buffer_size = 32 * 1204;
	std::vector<char> read_buffer;

	auto remaining = to_skip;

	size_t to_reserve = std::min<size_t>(remaining, c_read_buffer_size);

	read_buffer.reserve(to_reserve);

	while (remaining)
	{
		size_t to_read = std::min<size_t>(static_cast<size_t>(remaining), c_read_buffer_size);

		auto actual_read = raw_read_some(gsl::span<char>{read_buffer.data(), to_read});
		remaining -= actual_read;
	}

	m_read_offset += to_skip;
}

size_t io_utility::sequential_reader::read_some(gsl::span<char> buffer)
{
	auto actual_read = raw_read_some(buffer);
	m_read_offset += actual_read;

	return actual_read;
}
