/**
 * @file get_file_hash.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "get_file_hash.h"

#include "binary_file_reader.h"
#include "wrapped_reader_sequential_reader.h"

#include "hash_utility.h"

std::vector<char> get_file_hash(fs::path path)
{
	io_utility::binary_file_reader reader(path.string());
	io_utility::wrapped_reader_sequential_reader wrapped_reader(&reader);

	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	const size_t block_size = 1024 * 1024;
	std::vector<char> read_buffer;
	read_buffer.reserve(block_size);

	size_t actual_read;
	while (true)
	{
		actual_read = wrapped_reader.read_some(gsl::span<char>{read_buffer.data(), block_size});

		if (actual_read == 0)
		{
			break;
		}

		hasher.hash_data(std::string_view{read_buffer.data(), actual_read});
	}

	return hasher.get_hash_binary();
}