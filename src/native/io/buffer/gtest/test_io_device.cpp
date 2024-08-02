/**
 * @file test_io_device.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <memory>
#include <vector>

#include <io/buffer/io_device.h>
#include "common.h"

TEST(io_device, using_vector_capacity)
{
	auto data = std::make_shared<std::vector<char>>();

	data->reserve(c_reader_vector_size);

	using device = archive_diff::io::buffer::io_device;
	auto reader  = device::make_reader(data, device::size_kind::vector_capacity);

	test_buffer_reader(data, reader);
}

TEST(io_device, using_vector_size)
{
	auto data = std::make_shared<std::vector<char>>();

	data->resize(c_reader_vector_size);

	using device = archive_diff::io::buffer::io_device;
	auto reader  = device::make_reader(data, device::size_kind::vector_size);

	test_buffer_reader(data, reader);
}