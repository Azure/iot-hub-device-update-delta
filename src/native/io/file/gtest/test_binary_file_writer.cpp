/**
 * @file test_binary_file_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/reader_validation.h>
#include <test_utility/random_data_file.h>

#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
#include <hashing/hasher.h>

#include <iostream>
#include <fstream>

#include <language_support/include_filesystem.h>

void write_file_to_writer(fs::path file_path, archive_diff::io::writer &writer)
{
	auto offset = 0ull;

	auto remaining = fs::file_size(file_path);

	const size_t c_read_buffer_block_size = 32 * 1024;
	std::vector<char> read_buffer;
	read_buffer.reserve(c_read_buffer_block_size);

	std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);
	if (file_stream.bad())
	{
		std::cerr << "Failed to open " << file_path << std::endl;
		throw std::exception();
	}

	while (remaining)
	{
		size_t to_read = std::min<size_t>(c_read_buffer_block_size, static_cast<size_t>(remaining));

		file_stream.read(read_buffer.data(), to_read);

		writer.write(offset, std::string_view{read_buffer.data(), to_read});

		offset += to_read;
		remaining -= to_read;
	}
}

TEST(binary_file_writer, missing_directory)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer";
	fs::remove_all(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\sample.zst";

	bool caught_any_exception = false;
	auto error                = archive_diff::errors::error_code::none;
	// verify we can't open the file without the directory containing it existing
	try
	{
		archive_diff::io::file::binary_file_writer writer(test_file_relative.string());
	}
	catch (archive_diff::errors::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}
	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(archive_diff::errors::error_code::io_binary_file_writer_failed_open, error);
}

#ifdef WIN32
TEST(binary_file_writer, open_twice)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer";
	fs::remove_all(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\sample.zst";

	fs::create_directory(test_temp_path);

	archive_diff::io::file::binary_file_writer writer(test_file_relative.string());

	bool caught_any_exception = false;
	auto error                = archive_diff::errors::error_code::none;

	// verify we can't open another file at the same time
	try
	{
		archive_diff::io::file::binary_file_writer writer2(test_file_relative.string());
	}
	catch (archive_diff::errors::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(archive_diff::errors::error_code::io_binary_file_writer_failed_open, error);
}
#endif

TEST(binary_file_writer, write_file_and_verify)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer" / "write_file_and_verify";
	auto data_file_path = test_temp_path / "data_file.bin";

	fs::remove_all(test_temp_path);
	fs::path data_file_from_writer = test_temp_path / "data_file_from_writer.bin";

	{
		fs::create_directories(test_temp_path);
		std::srand(0);
		archive_diff::test_utility::create_random_data_file(data_file_path.string(), 100000);

		archive_diff::io::file::binary_file_writer writer(data_file_from_writer.string());
		write_file_to_writer(data_file_path, writer);
		writer.flush();
	}

	ASSERT_TRUE(archive_diff::test_utility::files_are_equal(data_file_from_writer.string(), data_file_path));
}
