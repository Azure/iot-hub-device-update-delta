/**
 * @file io_device_view_test.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/io_device_view.h>

#include "buffer_io_device.h"

TEST(io_device_view, buffer_io_device_alphabet)
{
	const char c_alphabet[]            = "abcdefghijklmnopqrstuvwxyz";
	const size_t c_letters_in_alphabet = sizeof(c_alphabet) - 1;

	archive_diff::io::shared_io_device device =
		std::make_shared<archive_diff::io::test::buffer_io_device>(std::string_view{c_alphabet, c_letters_in_alphabet});
	archive_diff::io::io_device_view view(device);

	std::vector<char> data;

	for (size_t i = 0; i <= c_letters_in_alphabet; i++)
	{
		data.resize(i);

		for (size_t j = 0; j <= c_letters_in_alphabet; j++)
		{
			auto actual_read = view.read_some(j, data);
			auto expected    = std::min<uint64_t>(c_letters_in_alphabet - j, static_cast<uint64_t>(i));
			ASSERT_EQ(actual_read, expected);
		}
	}
}
