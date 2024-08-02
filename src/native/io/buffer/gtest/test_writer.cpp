/**
 * @file test_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <memory>
#include <vector>

#include <io/buffer/writer.h>
#include "common.h"

TEST(writer, write_within_bounds)
{
	auto data = std::make_shared<std::vector<char>>();

	data->resize(c_reader_vector_size);

	archive_diff::io::buffer::writer writer(data);

	for (size_t i = 0; i < c_reader_vector_size - 1; i++)
	{
		for (size_t j = 1; i + j < c_reader_vector_size; j++)
		{
			// start with blank slate
			std::memset(data->data(), 0, c_reader_vector_size);

			std::vector<char> data_to_write;
			data_to_write.resize(j);
			std::memset(data_to_write.data(), 0xa, j);

			writer.write(i, std::string_view{data_to_write.data(), static_cast<size_t>(j)});
			ASSERT_EQ(0, std::memcmp(data_to_write.data(), data->data() + i, j));
		}
	}
}

TEST(writer, write_past_bound)
{
	auto data = std::make_shared<std::vector<char>>();

	data->resize(c_reader_vector_size);

	archive_diff::io::buffer::writer writer(data);

	for (size_t i = 0; i < c_reader_vector_size * 2; i++)
	{
		for (size_t j = 100; j < 1000; j++)
		{
			// start with blank slate
			data->resize(c_reader_vector_size);
			std::memset(data->data(), 0, c_reader_vector_size);
			ASSERT_EQ(data->size(), c_reader_vector_size);

			std::vector<char> data_to_write;
			data_to_write.resize(j);
			std::memset(data_to_write.data(), 0xa, j);
			writer.write(i, std::string_view{data_to_write.data(), static_cast<size_t>(j)});

			ASSERT_EQ(writer.size(), data->size());
			ASSERT_EQ(data->size(), std::max(c_reader_vector_size, i + j));
			ASSERT_EQ(0, std::memcmp(data_to_write.data(), data->data() + i, j));
		}
	}
}