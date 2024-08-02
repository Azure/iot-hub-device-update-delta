/**
 * @file common.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <cstring>

#include <test_utility/gtest_includes.h>

#include "common.h"

void test_buffer_reader(std::shared_ptr<std::vector<char>> data, archive_diff::io::reader &reader)
{
	std::memset(data->data(), 0, c_reader_vector_size);

	std::vector<char> result;
	reader.read_all(result);

	ASSERT_EQ(reader.size(), c_reader_vector_size);
	ASSERT_EQ(result.size(), c_reader_vector_size);

	ASSERT_EQ(0, std::memcmp(result.data(), data->data(), c_reader_vector_size));

	for (size_t i = 0; i < c_reader_vector_size - 1; i++)
	{
		for (size_t j = 1; i + j < c_reader_vector_size; j++)
		{
			// start with blank slate
			std::memset(data->data(), 0, c_reader_vector_size);

			std::memset(data->data() + i, 0xa, j);

			result.resize(j);
			reader.read(i, std::span<char>{result.data(), j});

			ASSERT_EQ(0, std::memcmp(result.data(), data->data() + i, j));
		}
	}
}
