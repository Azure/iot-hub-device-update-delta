/**
 * @file test_reader_factory.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <memory>
#include <vector>

#include <io/buffer/reader_factory.h>
#include "common.h"

TEST(reader_factory, using_vector_capacity)
{
	auto data = std::make_shared<std::vector<char>>();

	data->reserve(c_reader_vector_size);

	auto factory_instance =
		archive_diff::io::buffer::reader_factory{data, archive_diff::io::buffer::io_device::size_kind::vector_capacity};
	auto reader = factory_instance.make_reader();

	test_buffer_reader(data, reader);
}

TEST(reader_factory, using_vector_size)
{
	auto data = std::make_shared<std::vector<char>>();

	data->resize(c_reader_vector_size);

	using factory         = archive_diff::io::buffer::reader_factory;
	using device          = archive_diff::io::buffer::io_device;
	auto factory_instance = factory{data, device::size_kind::vector_size};
	auto reader           = factory_instance.make_reader();
	test_buffer_reader(data, reader);
}