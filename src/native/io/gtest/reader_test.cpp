/**
 * @file reader_test.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/reader.h>

#include "buffer_io_device.h"

static const char c_alphabet[]            = "abcdefghijklmnopqrstuvwxyz";
static const size_t c_letters_in_alphabet = sizeof(c_alphabet) - 1;

void test_reading_from_alphabet_reader(const archive_diff::io::reader &reader)
{
	std::vector<char> data;

	for (size_t to_read = 0; to_read <= c_letters_in_alphabet; to_read++)
	{
		data.resize(to_read);

		for (size_t offset = 0; offset <= c_letters_in_alphabet; offset++)
		{
			auto actual_read = reader.read_some(offset, data);
			auto expected    = std::min(c_letters_in_alphabet - offset, to_read);
			ASSERT_EQ(actual_read, expected);
			ASSERT_EQ(0, std::memcmp(data.data(), c_alphabet + offset, expected));
		}
	}
}

TEST(reader_simple, read_from_alphabet)
{
	std::shared_ptr<archive_diff::io::io_device> device =
		std::make_shared<archive_diff::io::test::buffer_io_device>(std::string_view{c_alphabet, c_letters_in_alphabet});
	archive_diff::io::io_device_view view{device};
	archive_diff::io::reader reader{view};

	test_reading_from_alphabet_reader(reader);
}

void test_slicing_alphabet_reader(const archive_diff::io::reader &reader)
{
	std::vector<char> data;

	for (size_t slice_offset_start = 0; slice_offset_start <= c_letters_in_alphabet; slice_offset_start++)
	{
		for (size_t slice_offset_end = slice_offset_start + 1; slice_offset_end <= c_letters_in_alphabet;
		     slice_offset_end++)
		{
			auto len   = slice_offset_end - slice_offset_start;
			auto slice = reader.slice(slice_offset_start, len);

			for (uint64_t to_read = 1; to_read <= len; to_read++)
			{
				data.resize(to_read);
				auto actual_read = slice.read_some(0, data);

				ASSERT_EQ(actual_read, to_read);
				ASSERT_EQ(0, std::memcmp(data.data(), c_alphabet + slice_offset_start, to_read));
			}
		}
	}
}

TEST(reader_simple, slice_alphabet)
{
	std::shared_ptr<archive_diff::io::io_device> device =
		std::make_shared<archive_diff::io::test::buffer_io_device>(std::string_view{c_alphabet, c_letters_in_alphabet});
	archive_diff::io::io_device_view view{device};
	archive_diff::io::reader reader{view};

	test_slicing_alphabet_reader(reader);
}

TEST(reader_chain, read_from_alphabet)
{
	// break the alphabet in two split a given position
	for (size_t split_pos = 1; split_pos < c_letters_in_alphabet; split_pos++)
	{
		std::string_view first{c_alphabet, split_pos};
		std::string_view second{c_alphabet + split_pos, c_letters_in_alphabet - split_pos};

		archive_diff::io::shared_io_device first_device =
			std::make_shared<archive_diff::io::test::buffer_io_device>(first);
		archive_diff::io::shared_io_device second_device =
			std::make_shared<archive_diff::io::test::buffer_io_device>(second);

		archive_diff::io::io_device_view first_view{first_device};
		archive_diff::io::io_device_view second_view{second_device};

		archive_diff::io::reader first_reader{first_view};
		archive_diff::io::reader second_reader{second_view};

		auto chain = first_reader.chain(second_reader);

		test_reading_from_alphabet_reader(chain);
	}
}

TEST(reader_chain, slice_alphabet)
{
	// break the alphabet in two split a given position
	for (size_t split_pos = 1; split_pos < c_letters_in_alphabet; split_pos++)
	{
		std::string_view first{c_alphabet, split_pos};
		std::string_view second{c_alphabet + split_pos, c_letters_in_alphabet - split_pos};

		archive_diff::io::shared_io_device first_device =
			std::make_shared<archive_diff::io::test::buffer_io_device>(first);
		archive_diff::io::shared_io_device second_device =
			std::make_shared<archive_diff::io::test::buffer_io_device>(second);

		archive_diff::io::io_device_view first_view{first_device};
		archive_diff::io::io_device_view second_view{second_device};

		archive_diff::io::reader first_reader{first_view};
		archive_diff::io::reader second_reader{second_view};

		auto chain = first_reader.chain(second_reader);

		test_slicing_alphabet_reader(chain);
	}
}