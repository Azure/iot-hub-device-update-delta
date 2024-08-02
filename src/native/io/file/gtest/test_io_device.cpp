/**
 * @file test_io_device.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/reader_validation.h>
#include <test_utility/random_data_file.h>

#include <io/reader.h>
#include <io/file/io_device.h>

#include <iostream>
#include <fstream>

#include <language_support/include_filesystem.h>

void compare_reader_and_file(archive_diff::io::reader &reader, fs::path test_file, uint64_t offset, uint64_t length)
{
	EXPECT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, test_file, offset, length, 1024));
	EXPECT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, test_file, offset, length, 32 * 1024));
	EXPECT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, test_file, offset, length, 64 * 1024));
	EXPECT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, test_file, offset, length, 1024 * 1024));
	EXPECT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, test_file, offset, length, length));
}

TEST(io_device, open_and_verify)
{
	auto test_temp_path = fs::temp_directory_path() / "io_device" / "open_and_verify";
	auto data_file_path = test_temp_path / "data_file.bin";

	fs::create_directories(test_temp_path);
	std::srand(0);
	archive_diff::test_utility::create_random_data_file(data_file_path.string(), 100000);

	auto file_size = fs::file_size(data_file_path);

	auto reader = archive_diff::io::file::io_device::make_reader(data_file_path.string());
	compare_reader_and_file(reader, data_file_path, 0, file_size);

	ASSERT_EQ(file_size, reader.size());
}

TEST(io_device, open_missing)
{
	bool caught_any_exception = false;
	auto error                = archive_diff::errors::error_code::none;

	try
	{
		auto reader = archive_diff::io::file::io_device::make_reader("this_path_does_not_exist_I_hope");
	}
	catch (archive_diff::errors::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(archive_diff::errors::error_code::io_binary_file_reader_failed_open, error);
}

TEST(io_device, missing_directory)
{
	auto test_temp_path = fs::temp_directory_path() / "io_device" / "missing_directory";
	fs::remove_all(test_temp_path);

	fs::path path = test_temp_path / "a\\b\\c\\..\\..\\..\\sample.zst";

	bool caught_any_exception = false;
	auto error                = archive_diff::errors::error_code::none;
	// verify we can't open the file without the directory containing it existing
	try
	{
		auto reader = archive_diff::io::file::io_device::make_reader(path.string());
	}
	catch (archive_diff::errors::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(archive_diff::errors::error_code::io_binary_file_reader_failed_open, error);
}

TEST(io_device, verify_slices)
{
	auto test_temp_path = fs::temp_directory_path() / "io_device" / "verify_slices";
	auto data_file_path = test_temp_path / "data_file.bin";

	fs::create_directories(test_temp_path);
	std::srand(0);
	archive_diff::test_utility::create_random_data_file(data_file_path.string(), 100000);

	auto reader = archive_diff::io::file::io_device::make_reader(data_file_path.string());

	std::pair<uint64_t, uint64_t> test_slices[] = {
		{3000, 50000},
		{300, 500},
		{0, fs::file_size(data_file_path)},
		{10000, 20},
	};

	for (auto &test_slice : test_slices)
	{
		auto offset = test_slice.first;
		auto length = test_slice.second;
		auto slice  = reader.slice(offset, length);
		ASSERT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(slice, data_file_path, offset, length, 1024));
	}
}

TEST(io_device, more_slice_tests)
{
	auto test_temp_path = fs::temp_directory_path() / "io_device" / "more_slice_tests";
	auto data_file_path = test_temp_path / "data_file.bin";

	fs::create_directories(test_temp_path);
	std::srand(0);
	archive_diff::test_utility::create_random_data_file(data_file_path.string(), 100000);

	auto file_size = fs::file_size(data_file_path);
	auto reader    = archive_diff::io::file::io_device::make_reader(data_file_path.string());

	ASSERT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(reader, data_file_path, 0, file_size, 4096));

	std::pair<uint64_t, uint64_t> offset_and_length[] = {
		{100, 10000}, {60000, 1000}, {100, file_size - 1000}, {23 * 1024, 32 * 1024}};

	for (const auto &values : offset_and_length)
	{
		auto offset = values.first;
		auto length = values.second;
		auto slice  = reader.slice(offset, length);
		ASSERT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(slice, data_file_path, offset, length, 4096));
	}

	uint64_t offsets[] = {100, 10313, 39921, 10231, 62 * 1024};

	for (auto offset : offsets)
	{
		auto length = file_size - offset;
		auto slice  = reader.slice(offset, length);
		ASSERT_TRUE(archive_diff::test_utility::reader_and_file_are_equal(slice, data_file_path, offset, length, 4096));
	}

	uint64_t lengths[] = {
		1000,
		20000,
		3000,
		41239,
		70000,
	};

	for (auto length : lengths)
	{
		auto offset = 0ull;
		auto slice  = reader.slice(offset, length);

		compare_reader_and_file(slice, data_file_path, offset, length);
	}

	for (const auto &values : offset_and_length)
	{
		auto offset = values.first;
		auto length = values.second;
		auto slice  = reader.slice(offset, length);
		compare_reader_and_file(slice, data_file_path, offset, length);
	}

	for (auto offset : offsets)
	{
		auto length = file_size - offset;
		auto slice  = reader.slice(offset, length);
		compare_reader_and_file(slice, data_file_path, offset, length);
	}

	auto whole_file_slice = reader.slice(0, file_size);
	compare_reader_and_file(whole_file_slice, data_file_path, 0, file_size);
}
