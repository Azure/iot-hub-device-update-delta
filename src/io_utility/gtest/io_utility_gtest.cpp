/**
 * @file io_utility_gtest.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>

#include "binary_file_reader.h"
#include "binary_file_readerwriter.h"
#include "binary_file_writer.h"
#include "child_reader.h"

#include "wrapped_writer_sequential_hashed_writer.h"
#include "child_hashed_writer.h"

#include "hash_utility.h"

#include <vector>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

#include "gtest/gtest.h"
using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

const fs::path c_source_boot_file_compressed = "source/boot.zst";

fs::path test_data_root;

fs::path get_data_file(const fs::path file)
{
	auto path = test_data_root / file;
	return path;
}

bool reader_and_file_are_equal(
	io_utility::reader &reader, fs::path file_path, uint64_t offset, uint64_t length, size_t chunk_size)
{
	std::vector<char> reader_data;
	std::vector<char> file_data;

	reader_data.reserve(chunk_size);
	file_data.reserve(chunk_size);

	std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);

	file_stream.seekg(offset);

	auto reader_offset = 0ull;
	auto remaining     = length;

	while (remaining)
	{
		size_t to_read = std::min<size_t>(chunk_size, static_cast<size_t>(remaining));

		file_stream.read(file_data.data(), to_read);
		auto actual_read = file_stream.gcount();

		if (actual_read == 0)
		{
			break;
		}

		reader.read(reader_offset, gsl::span<char>{reader_data.data(), static_cast<size_t>(actual_read)});

		if (0 != memcmp(reader_data.data(), file_data.data(), to_read))
		{
			return false;
		}

		remaining -= to_read;
		reader_offset += to_read;
	}

	return true;
}

void compare_reader_and_file(io_utility::reader &reader, fs::path test_file, uint64_t offset, uint64_t length)
{
	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 1024));
	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 32 * 1024));
	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 64 * 1024));
	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 1024 * 1024));
	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, length));
}

TEST(binary_file_reader, open_and_verify)
{
	auto source_file_compressed = get_data_file(c_source_boot_file_compressed);

	auto file_size = fs::file_size(source_file_compressed);
	io_utility::binary_file_reader reader(source_file_compressed.string());
	compare_reader_and_file(reader, source_file_compressed, 0, file_size);
	ASSERT_EQ(file_size, reader.size());
}

TEST(binary_file_reader, open_missing)
{
	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;

	try
	{
		io_utility::binary_file_reader broken_reader("this_path_does_not_exist_I_hope");
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_reader_failed_open, error);
}

TEST(binary_file_readerwriter, missing_directory)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_readerwriter";
	fs::remove_all(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\boot.zst";

	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;
	// verify we can't open the file without the directory containing it existing
	try
	{
		io_utility::binary_file_readerwriter reader(test_file_relative.string());
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_readerwriter_failed_open, error);
}

#ifdef WIN32
TEST(binary_file_readerwriter, open_twice)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_readerwriter";
	fs::remove_all(test_temp_path);

	fs::create_directory(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\boot.zst";

	io_utility::binary_file_readerwriter reader(test_file_relative.string());

	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;

	// verify we can't open another file at the same time
	try
	{
		io_utility::binary_file_readerwriter reader(test_file_relative.string());
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_readerwriter_failed_open, error);
}
#endif

void write_file_to_writer(fs::path file_path, io_utility::writer &writer)
{
	auto offset = 0ull;

	auto remaining = fs::file_size(file_path);

	const size_t c_read_buffer_block_size = 32 * 1024;
	std::vector<char> read_buffer;
	read_buffer.reserve(c_read_buffer_block_size);

	std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);

	while (remaining)
	{
		size_t to_read = std::min<size_t>(c_read_buffer_block_size, static_cast<size_t>(remaining));

		file_stream.read(read_buffer.data(), to_read);

		writer.write(offset, std::string_view{read_buffer.data(), to_read});

		offset += to_read;
		remaining -= to_read;
	}
}

bool files_are_equal(fs::path left, fs::path right)
{
	if (!fs::exists(left) || !fs::exists(right))
	{
		return false;
	}

	auto left_size  = fs::file_size(left);
	auto right_size = fs::file_size(right);

	if (left_size != right_size)
	{
		return false;
	}

	const size_t c_read_buffer_block_size = 32 * 1024;

	std::vector<char> read_buffer_left;
	std::vector<char> read_buffer_right;
	read_buffer_left.reserve(c_read_buffer_block_size);
	read_buffer_right.reserve(c_read_buffer_block_size);

	std::ifstream file_stream_left(left, std::ios::binary | std::ios::in);
	std::ifstream file_stream_right(left, std::ios::binary | std::ios::in);

	auto remaining = left_size;

	while (remaining)
	{
		size_t to_read = std::min<size_t>(c_read_buffer_block_size, remaining);

		file_stream_left.read(read_buffer_left.data(), to_read);
		file_stream_right.read(read_buffer_right.data(), to_read);

		if (0 != memcmp(read_buffer_left.data(), read_buffer_right.data(), to_read))
		{
			return false;
		}

		remaining -= to_read;
	}

	return true;
}

TEST(binary_file_readerwriter, write_file_and_verify)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_readerwriter";
	fs::remove_all(test_temp_path);

	fs::create_directory(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\boot.zst";

	fs::path test_file_absolute = test_file_relative.lexically_normal();

	io_utility::binary_file_readerwriter readerwriter(test_file_absolute.string());

	ASSERT_EQ(0ull, readerwriter.size());

	auto source_file_compressed = get_data_file(c_source_boot_file_compressed);

	write_file_to_writer(source_file_compressed, readerwriter);
	auto file_size = fs::file_size(source_file_compressed);
	ASSERT_EQ(file_size, readerwriter.size());

	readerwriter.flush();

	compare_reader_and_file(readerwriter, source_file_compressed, 0, file_size);
	ASSERT_TRUE(files_are_equal(source_file_compressed, test_file_absolute));
}

#define INVALID_FILE_NAME "<this_path_does_not_exist_I_hope/"

TEST(binary_file_readerwriter, open_invalid_file_name)
{
	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;

	try
	{
		io_utility::binary_file_readerwriter broken_readerwriter(INVALID_FILE_NAME);
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_readerwriter_failed_open, error);
}

TEST(binary_file_writer, missing_directory)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer";
	fs::remove_all(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\boot.zst";

	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;
	// verify we can't open the file without the directory containing it existing
	try
	{
		io_utility::binary_file_writer writer(test_file_relative.string());
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}
	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_writer_failed_open, error);
}

#ifdef WIN32
TEST(binary_file_writer, open_twice)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer";
	fs::remove_all(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a\\b\\c\\..\\..\\..\\boot.zst";

	fs::create_directory(test_temp_path);

	io_utility::binary_file_writer writer(test_file_relative.string());

	bool caught_any_exception = false;
	auto error                = error_utility::error_code::none;

	// verify we can't open another file at the same time
	try
	{
		io_utility::binary_file_writer writer2(test_file_relative.string());
	}
	catch (error_utility::user_exception &e)
	{
		caught_any_exception = true;
		error                = e.get_error();
	}

	ASSERT_TRUE(caught_any_exception);
	ASSERT_EQ(error_utility::error_code::io_binary_file_writer_failed_open, error);
}
#endif

TEST(binary_file_writer, write_file_and_verify)
{
	auto test_temp_path = fs::temp_directory_path() / "binary_file_writer";
	fs::remove_all(test_temp_path);
	fs::create_directory(test_temp_path);

	fs::path test_file_relative = test_temp_path / "a/b/c/../../../boot.zst";
	fs::path test_file_absolute = test_file_relative.lexically_normal();
	io_utility::binary_file_writer writer(test_file_absolute.string());
	auto source_file_compressed = get_data_file(c_source_boot_file_compressed);
	write_file_to_writer(source_file_compressed, writer);
	writer.flush();
	ASSERT_TRUE(files_are_equal(source_file_compressed, test_file_absolute));
}

TEST(child_reader, using_binary_file_reader)
{
	auto source_file_compressed = get_data_file(c_source_boot_file_compressed);

	auto file_size = fs::file_size(source_file_compressed);
	io_utility::binary_file_reader file_reader(source_file_compressed.string());
	compare_reader_and_file(file_reader, source_file_compressed, 0, file_size);

	io_utility::child_reader child_reader_whole(&file_reader);

	compare_reader_and_file(child_reader_whole, c_source_boot_file_compressed, 0, file_size);

	std::pair<uint64_t, uint64_t> offset_and_length[] = {
		{100, 10000}, {1000000, 1000}, {100, file_size - 1000}, {62 * 1024, 132 * 1024}};

	for (const auto &values : offset_and_length)
	{
		auto offset = values.first;
		auto length = values.second;
		io_utility::child_reader reader(&file_reader, offset, length);
		compare_reader_and_file(reader, c_source_boot_file_compressed, offset, length);
	}

	uint64_t offets[] = {100, 10313, 39921, 10231, 62 * 1024, 321 * 1024};

	for (auto offset : offets)
	{
		auto length = file_size - offset;
		auto reader = io_utility::child_reader::with_base_offset(&file_reader, offset);
		compare_reader_and_file(*reader.get(), c_source_boot_file_compressed, offset, length);
	}

	uint64_t lengths[] = {103103, 1301313, 1323113, 100, 1313141, 12214122, 1124102931, 999999};

	for (auto length : lengths)
	{
		auto offset = 0ull;
		auto reader = io_utility::child_reader::with_length(&file_reader, length);
		compare_reader_and_file(*reader.get(), c_source_boot_file_compressed, offset, length);
	}

	for (const auto &values : offset_and_length)
	{
		auto offset = values.first;
		auto length = values.second;
		io_utility::child_reader reader(&file_reader, std::optional<uint64_t>{offset}, std::optional<uint64_t>{length});
		compare_reader_and_file(reader, c_source_boot_file_compressed, offset, length);
	}

	for (auto offset : offets)
	{
		auto length = file_size - offset;
		io_utility::child_reader reader(&file_reader, std::optional<uint64_t>{offset}, std::optional<uint64_t>{});
		compare_reader_and_file(reader, c_source_boot_file_compressed, offset, length);
	}

	for (auto length : lengths)
	{
		auto offset = 0ull;
		io_utility::child_reader reader(&file_reader, std::optional<uint64_t>{}, std::optional<uint64_t>{length});
		compare_reader_and_file(reader, c_source_boot_file_compressed, offset, length);
	}

	{
		io_utility::child_reader reader(&file_reader, std::optional<uint64_t>{}, std::optional<uint64_t>{});
		compare_reader_and_file(reader, c_source_boot_file_compressed, 0, file_size);
	}
}

std::vector<char> get_hash(const char *data, size_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	std::string_view view{data, length};
	hasher.hash_data(view);

	return hasher.get_hash_binary();
}

std::vector<char> get_hash(io_utility::reader *reader, uint64_t offset, uint64_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	std::vector<char> buffer;
	buffer.resize(32 * 1024);

	auto remaining = length;

	auto current_offset = offset;

	while (remaining > 0)
	{
		size_t to_read = std::min<size_t>(remaining, buffer.capacity());

		reader->read(current_offset, gsl::span<char>{buffer.data(), to_read});

		hasher.hash_data(std::string_view{buffer.data(), static_cast<size_t>(to_read)});

		current_offset += to_read;
		remaining -= to_read;
	}

	return hasher.get_hash_binary();
}

std::vector<char> get_hash(fs::path path, uint64_t offset, uint64_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	io_utility::binary_file_reader reader(path.string());

	return get_hash(&reader, offset, static_cast<size_t>(length));
}

TEST(writer_wrapper_sequential_hashed_writer, binary_file_writer)
{
	char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	auto hash = get_hash(data, sizeof(data));

	auto test_temp_path = fs::temp_directory_path() / "writer_wrapper_sequential_hashed_writer";
	fs::remove_all(test_temp_path);
	fs::create_directories(test_temp_path);

	auto test_file = test_temp_path / "target";

	io_utility::binary_file_writer binary_writer(test_file.string());

	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	io_utility::wrapped_writer_sequential_hashed_writer hashed_writer(&binary_writer, &hasher);

	std::string_view buffer{data, sizeof(data)};

	hashed_writer.write(buffer);

	ASSERT_EQ(sizeof(data), hashed_writer.tellp());

	auto calculated_hash = hasher.get_hash_binary();

	ASSERT_EQ(hash.size(), calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), calculated_hash.data(), hash.size()));
}

TEST(child_hashed_writer, wrapped_writer_sequential_hashed_writer)
{
	char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	auto hash = get_hash(data, sizeof(data));

	auto test_temp_path = fs::temp_directory_path() / "child_hashed_writer";
	fs::remove_all(test_temp_path);
	fs::create_directories(test_temp_path);

	auto test_file = test_temp_path / "target";

	io_utility::binary_file_writer binary_writer(test_file.string());

	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	io_utility::wrapped_writer_sequential_hashed_writer hashed_writer(&binary_writer, &hasher);

	hash_utility::hasher child_hasher(hash_utility::algorithm::SHA256);
	io_utility::child_hashed_writer child_hashed_writer(&hashed_writer, &child_hasher);

	std::string_view buffer{data, sizeof(data)};

	child_hashed_writer.write(buffer);

	ASSERT_EQ(sizeof(data), hashed_writer.tellp());
	ASSERT_EQ(sizeof(data), child_hashed_writer.tellp());

	auto calculated_hash       = hasher.get_hash_binary();
	auto child_calculated_hash = child_hasher.get_hash_binary();

	ASSERT_EQ(hash.size(), calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), calculated_hash.data(), hash.size()));

	ASSERT_EQ(hash.size(), child_calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), child_calculated_hash.data(), hash.size()));
}

int main(int argc, char **argv)
{
	if (argc > 2 && strcmp(argv[1], "--test_data_root") == 0)
	{
		test_data_root = argv[2];
	}
	else
	{
		printf("Must specify test data root with --test_data_root <path>\n");
		return 1;
	}

	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}