/**
 * @file sequential_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "sequential_writer.h"
#include "wrapped_reader_sequential_reader.h"

#include <vector>

void io_utility::sequential_writer::write(io_utility::reader *reader)
{
	io_utility::wrapped_reader_sequential_reader sequential_reader(reader);

	write(&sequential_reader);
}

void io_utility::sequential_writer::write(io_utility::sequential_reader *reader)
{
	const size_t c_read_buffer_max_capacity = 32 * 1024;
	std::vector<char> read_buffer;

	auto remaining = reader->size();

	auto to_reserve = std::min<size_t>(c_read_buffer_max_capacity, remaining);
	read_buffer.reserve(to_reserve);

	while (remaining)
	{
		auto to_read = std::min<size_t>(to_reserve, remaining);

		reader->read(gsl::span<char>{read_buffer.data(), to_read});
		write(std::string_view{read_buffer.data(), to_read});

		remaining -= to_read;
	}
}

void io_utility::sequential_writer_impl::write(uint64_t offset, std::string_view buffer)
{
	if (offset != m_offset)
	{
		std::string msg = "Attempting to write to offset: " + std::to_string(offset)
		                + " but current offset is: " + std::to_string(m_offset);
		throw error_utility::user_exception(error_utility::error_code::io_sequential_write_bad_offset, msg);
	}

	write(buffer);
}

void io_utility::sequential_writer_impl::write(std::string_view buffer)
{
	write_impl(buffer);
	m_offset += buffer.size();
}

uint64_t io_utility::sequential_writer_impl::tellp() { return m_offset; }
