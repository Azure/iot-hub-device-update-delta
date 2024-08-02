/**
 * @file io_device_test.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/io_device.h>

#include "buffer_io_device.h"

TEST(io_device, buffer_io_device_alphabet)
{
	const char c_alphabet[]            = "abcdefghijklmnopqrstuvwxyz";
	const size_t c_letters_in_alphabet = sizeof(c_alphabet) - 1;
	archive_diff::io::test::buffer_io_device device{std::string_view{c_alphabet, c_letters_in_alphabet}};

	std::vector<char> data;

	for (size_t i = 0; i <= c_letters_in_alphabet; i++)
	{
		data.resize(i);

		for (size_t j = 0; j <= c_letters_in_alphabet; j++)
		{
			auto actual_read = device.read_some(j, data);
			auto expected =
				static_cast<size_t>(std::min<uint64_t>(c_letters_in_alphabet - j, static_cast<uint64_t>(i)));
			ASSERT_EQ(actual_read, expected);
		}
	}
}
