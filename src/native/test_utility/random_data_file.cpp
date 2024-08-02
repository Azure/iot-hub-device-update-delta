/**
 * @file random_data_file.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "random_data_file.h"

#include <iostream>
#include <fstream>

namespace archive_diff::test_utility
{

void create_random_data_file(const std::string &path, uint64_t size)
{
	std::ofstream out_file(path);

	if (!out_file.is_open())
	{
		throw std::exception();
	}

	auto remaining = size;

	const size_t BLOCK_SIZE = 8 * 1024;
	char data[BLOCK_SIZE];

	while (remaining)
	{
		auto to_write = static_cast<size_t>(std::min<uint64_t>(BLOCK_SIZE, remaining));

		for (size_t i = 0; i < to_write; i++)
		{
			data[i] = std::rand() % 256;
		}

		out_file.write(data, to_write);

		remaining -= to_write;
	}
}
} // namespace archive_diff::test_utility