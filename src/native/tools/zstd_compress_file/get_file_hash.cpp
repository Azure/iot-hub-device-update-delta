/**
 * @file get_file_hash.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "get_file_hash.h"

#include <io/file/io_device.h>
#include <io/sequential/basic_reader_wrapper.h>

#include <hashing/hasher.h>

std::vector<char> get_file_hash(fs::path path)
{
	auto reader = archive_diff::io::file::io_device::make_reader(path.string());
	archive_diff::io::sequential::basic_reader_wrapper wrapped_reader(reader);

	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);

	const size_t block_size = 1024 * 1024;
	std::vector<char> read_buffer;
	read_buffer.reserve(block_size);

	size_t actual_read;
	while (true)
	{
		actual_read = wrapped_reader.read_some(std::span<char>{read_buffer.data(), block_size});

		if (actual_read == 0)
		{
			break;
		}

		hasher.hash_data(std::string_view{read_buffer.data(), actual_read});
	}

	return hasher.get_hash_binary();
}