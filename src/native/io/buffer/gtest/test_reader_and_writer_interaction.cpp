/**
 * @file test_reader_and_writer_interaction.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <memory>
#include <vector>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include "common.h"

void test_buffer_reader_with_writer(
	std::shared_ptr<std::vector<char>> data, archive_diff::io::reader &reader, archive_diff::io::writer *writer)
{
	std::memset(data->data(), 0, c_reader_vector_size);

	std::vector<char> result;
	reader.read_all(result);

	ASSERT_EQ(reader.size(), c_reader_vector_size);
	ASSERT_EQ(result.size(), c_reader_vector_size);

	ASSERT_EQ(0, std::memcmp(result.data(), data->data(), c_reader_vector_size));

	for (size_t i = 0; i < c_reader_vector_size * 2; i++)
	{
		for (size_t j = 0; j < 1000; j++)
		{
			// start with blank slate
			data->resize(c_reader_vector_size, 0);

			std::vector<char> to_write_data;
			to_write_data.resize(j);
			std::memset(to_write_data.data(), 0xa, j);
			writer->write(i, std::string_view{to_write_data.data(), to_write_data.size()});

			ASSERT_EQ(writer->size(), reader.size());
			ASSERT_EQ(writer->size(), data->size());
			ASSERT_EQ(writer->size(), std::max(c_reader_vector_size, i + j));

			result.resize(j);
			reader.read(i, std::span<char>{result.data(), j});

			ASSERT_EQ(j, result.size());
			ASSERT_EQ(0, std::memcmp(result.data(), data->data() + i, j));
			ASSERT_EQ(0, std::memcmp(result.data(), to_write_data.data(), j));
		}
	}
}

TEST(reader_and_writer_interaction, write_and_then_read)
{
	auto data = std::make_shared<std::vector<char>>();

	data->resize(c_reader_vector_size);

	using device = archive_diff::io::buffer::io_device;
	auto reader  = device::make_reader(data, device::size_kind::vector_size);
	archive_diff::io::buffer::writer writer{data};

	test_buffer_reader_with_writer(data, reader, &writer);
}