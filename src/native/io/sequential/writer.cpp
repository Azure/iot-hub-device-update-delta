/**
 * @file writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "writer.h"
#include "basic_reader_wrapper.h"

#include <vector>

#ifdef WIN32
	#include <winsock2.h>
#else
	#include "../uint64_t_endian.h"

	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

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

void writer::write_uint8_t(uint8_t value) { write(std::string_view{reinterpret_cast<char *>(&value), sizeof(value)}); }

void writer::write_uint16_t(uint16_t value)
{
	value = htons(value);
	write(std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}

void writer::write_uint32_t(uint32_t value)
{
	value = htonl(value);
	write(std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}

void writer::write_uint64_t(uint64_t value)
{
	value = htonll(value);
	write(std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}
} // namespace archive_diff::io::sequential