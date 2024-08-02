/**
 * @file reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <limits>
#include <vector>

#include <span>

#include "reader.h"

#include "user_exception.h"

namespace archive_diff::io::sequential
{
void reader::read(std::span<char> buffer)
{
	size_t actual = read_some(buffer);
	if (actual != buffer.size())
	{
		std::string msg = "Wanted to read " + std::to_string(buffer.size()) + " bytes, but could only read "
		                + std::to_string(actual) + " bytes.";
		throw errors::user_exception(errors::error_code::io_reader_read_failure, msg);
	}
}

void reader::read(std::string *value)
{
	uint64_t size;
	read(&size);
	if (size > std::numeric_limits<size_t>::max())
	{
		throw std::exception();
	}

	auto value_size = static_cast<size_t>(size);
	value->resize(value_size);
	read(std::span<char>{value->data(), value_size});
}

void reader::read_all_remaining(std::vector<char> &buffer)
{
	if (tellg() > size())
	{
		std::string msg = "reader::read_all_remaining: tellg() > size(). tellg: " + std::to_string(tellg())
		                + ", size(): " + std::to_string(size());
		throw errors::user_exception(errors::error_code::io_sequential_reader_bad_offset, msg);
	}

	auto to_read = size() - tellg();
	if (size() > std::numeric_limits<std::span<char>::size_type>::max())
	{
		std::string msg =
			"sequential::reader::read_all_remaining(): Attempting to read all data, but size() - tellg() is larger "
			"than numeric limit."
			" reader size: "
			+ std::to_string(size()) + ", tellg(): " + std::to_string(tellg())
			+ ", size() - tellg(): " + std::to_string(to_read)
			+ ", size_type::max(): " + std::to_string(std::numeric_limits<std::span<char>::size_type>::max());

		throw errors::user_exception(errors::error_code::io_zstd_dictionary_too_large, msg);
	}

	auto data_size = static_cast<std::span<char>::size_type>(to_read);
	buffer.resize(data_size);
	read(std::span<char>{buffer.data(), data_size});
}

void reader::skip_by_reading(uint64_t to_skip)
{
	const size_t c_read_buffer_size = 32 * 1204;
	std::vector<char> read_buffer;

	auto remaining = to_skip;

	size_t to_reserve = std::min<size_t>(remaining, c_read_buffer_size);

	read_buffer.reserve(to_reserve);

	while (remaining)
	{
		size_t to_read = std::min<size_t>(static_cast<size_t>(remaining), c_read_buffer_size);

		auto actual_read = read_some(std::span<char>{read_buffer.data(), to_read});
		remaining -= actual_read;
	}
}

} // namespace archive_diff::io::sequential
