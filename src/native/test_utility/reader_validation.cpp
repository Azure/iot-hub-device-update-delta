/**
 * @file reader_validation.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "reader_validation.h"

#include <cstring>
#include <iostream>
#include <fstream>

namespace archive_diff::test_utility
{
bool reader_and_file_are_equal(
	io::reader &reader, fs::path file_path, uint64_t offset, uint64_t length, size_t chunk_size)
{
	std::vector<char> reader_data;
	std::vector<char> file_data;

	reader_data.reserve(chunk_size);
	file_data.reserve(chunk_size);

	std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);
	if (!file_stream.is_open())
	{
		throw std::exception();
	}

	file_stream.seekg(offset);

	auto reader_offset = 0ull;
	auto remaining     = length;

	while (remaining)
	{
		size_t to_read = std::min<size_t>(chunk_size, static_cast<size_t>(remaining));

		file_stream.read(file_data.data(), to_read);
		auto actual_read = file_stream.gcount();

		if (actual_read == 0)
		{
			break;
		}

		reader.read(reader_offset, std::span<char>{reader_data.data(), static_cast<size_t>(actual_read)});

		if (0 != memcmp(reader_data.data(), file_data.data(), to_read))
		{
			return false;
		}

		remaining -= to_read;
		reader_offset += to_read;
	}

	return true;
}

bool files_are_equal(fs::path left, fs::path right)
{
	if (!fs::exists(left) || !fs::exists(right))
	{
		if (!fs::exists(left))
		{
			std::cerr << left << " does not exist." << std::endl;
		}

		if (!fs::exists(right))
		{
			std::cerr << right << " does not exist." << std::endl;
		}
		return false;
	}

	auto left_size  = fs::file_size(left);
	auto right_size = fs::file_size(right);

	if (left_size != right_size)
	{
		std::cerr << left << " and " << right << " have different sizes: " << std::to_string(left_size) << " and "
				  << std::to_string(right_size)
				  << std::endl;
		return false;
	}

	const size_t c_read_buffer_block_size = 32 * 1024;

	std::vector<char> read_buffer_left;
	std::vector<char> read_buffer_right;
	read_buffer_left.reserve(c_read_buffer_block_size);
	read_buffer_right.reserve(c_read_buffer_block_size);

	std::ifstream file_stream_left(left, std::ios::binary | std::ios::in);
	std::ifstream file_stream_right(left, std::ios::binary | std::ios::in);

	if (file_stream_left.bad())
	{
		std::cerr << "Failed to open: " << left << std::endl;
		return false;
	}

	if (file_stream_right.bad())
	{
		std::cerr << "Failed to open: " << right << std::endl;
		return false;
	}

	auto remaining = left_size;

	size_t offset = 0;
	while (remaining)
	{
		size_t to_read = std::min<size_t>(c_read_buffer_block_size, remaining);

		file_stream_left.read(read_buffer_left.data(), to_read);
		file_stream_right.read(read_buffer_right.data(), to_read);

		if (0 != memcmp(read_buffer_left.data(), read_buffer_right.data(), to_read))
		{
			std::cerr << "Content different at offset: " << std::to_string(offset) << std::endl;
			return false;
		}

		remaining -= to_read;
		offset += to_read;
	}

	return true;
}
} // namespace archive_diff::test_utility