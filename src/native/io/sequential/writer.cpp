/**
 * @file writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "writer.h"
#include "basic_reader_wrapper.h"

#include <vector>

namespace archive_diff::io::sequential
{
void writer::write(const io::reader &reader)
{
	const size_t c_read_buffer_max_capacity = 32 * 1024;
	std::vector<char> read_buffer;

	auto remaining = reader.size();

	auto to_reserve = std::min<size_t>(c_read_buffer_max_capacity, remaining);
	read_buffer.reserve(to_reserve);

	uint64_t offset{0};

	while (remaining)
	{
		auto to_read = std::min<size_t>(to_reserve, remaining);

		reader.read(offset, std::span<char>{read_buffer.data(), to_read});
		write(std::string_view{read_buffer.data(), to_read});

		remaining -= to_read;
		offset += to_read;
	}
}

void writer::write(io::sequential::reader &reader)
{
	const size_t c_read_buffer_max_capacity = 32 * 1024;
	std::vector<char> read_buffer;

	auto remaining = reader.size();

	auto to_reserve = std::min<size_t>(c_read_buffer_max_capacity, remaining);
	read_buffer.reserve(to_reserve);

	while (remaining)
	{
		auto to_read = std::min<size_t>(to_reserve, remaining);

		reader.read(std::span<char>{read_buffer.data(), to_read});
		write(std::string_view{read_buffer.data(), to_read});

		remaining -= to_read;
	}
}
} // namespace archive_diff::io::sequential