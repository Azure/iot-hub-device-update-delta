/**
 * @file nul_io_device_test.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/nul_device_reader.h>

TEST(nul_io_device, read_some)
{
	archive_diff::io::nul_io_device device;

	std::vector<char> data;

	for (int i = 0; i < 1000; i++)
	{
		data.resize(i);
		for (int j = 0; j < 1000; j++)
		{
			auto actual_read = device.read_some(j, data);
			ASSERT_EQ(actual_read, 0);
		}
	}
}