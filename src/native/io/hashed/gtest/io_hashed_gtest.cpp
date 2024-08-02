/**
 * @file io_hashed_gtest.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <fstream>

#if __has_include(<filesystem>)
	#include <filesystem>
namespace fs = std::filesystem;
#else
	#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>

#include <io/hashed/hashed_sequential_writer.h>

#include <io/reader.h>

#include <hashing/hasher.h>

#include "gtest/gtest.h"
using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

const fs::path c_sample_file_zst_compressed = "sample.zst";

fs::path test_data_root;

std::vector<char> get_hash(const char *data, size_t length)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
	std::string_view view{data, length};
	hasher.hash_data(view);

	return hasher.get_hash_binary();
}

std::vector<char> get_hash(archive_diff::io::reader *reader, uint64_t offset, uint64_t length)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
	std::vector<char> buffer;
	buffer.resize(32 * 1024);

	auto remaining = length;

	auto current_offset = offset;

	while (remaining > 0)
	{
		size_t to_read = std::min<size_t>(remaining, buffer.capacity());

		reader->read(current_offset, std::span<char>{buffer.data(), to_read});

		hasher.hash_data(std::string_view{buffer.data(), static_cast<size_t>(to_read)});

		current_offset += to_read;
		remaining -= to_read;
	}

	return hasher.get_hash_binary();
}

std::vector<char> get_hash(fs::path path, uint64_t offset, uint64_t length)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);

	auto reader = archive_diff::io::file::io_device::make_reader(path.string());
	return get_hash(&reader, offset, static_cast<size_t>(length));
}

TEST(hashed_sequential_writer, binary_file_writer)
{
	char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	auto hash = get_hash(data, sizeof(data));

	auto test_temp_path = fs::temp_directory_path() / "hashed_sequential_writer.binary_file_writer";
	fs::remove_all(test_temp_path);
	fs::create_directories(test_temp_path);

	auto test_file = test_temp_path / "target";

	std::shared_ptr<archive_diff::io::writer> binary_writer =
		std::make_shared<archive_diff::io::file::binary_file_writer>(test_file.string());
	auto hasher = std::make_shared<archive_diff::hashing::hasher>(archive_diff::hashing::algorithm::sha256);

	archive_diff::io::hashed::hashed_sequential_writer hashed_writer(binary_writer, hasher);

	std::string_view buffer{data, sizeof(data)};

	hashed_writer.write(buffer);

	ASSERT_EQ(sizeof(data), hashed_writer.tellp());

	auto calculated_hash = hasher->get_hash_binary();

	ASSERT_EQ(hash.size(), calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), calculated_hash.data(), hash.size()));
}

TEST(hashed_sequential_writer, child_hasher)
{
	char data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	auto hash = get_hash(data, sizeof(data));

	auto test_temp_path = fs::temp_directory_path() / "hashed_sequential_writer.child_hasher";
	fs::remove_all(test_temp_path);
	fs::create_directories(test_temp_path);

	auto test_file = test_temp_path / "target";

	std::shared_ptr<archive_diff::io::writer> binary_writer =
		std::make_shared<archive_diff::io::file::binary_file_writer>(test_file.string());
	auto hasher = std::make_shared<archive_diff::hashing::hasher>(archive_diff::hashing::algorithm::sha256);

	archive_diff::io::hashed::hashed_sequential_writer hashed_writer(binary_writer, hasher);

	auto child_hasher = std::make_shared<archive_diff::hashing::hasher>(archive_diff::hashing::algorithm::sha256);

	archive_diff::io::hashed::hashed_sequential_writer child_hashed_writer(hashed_writer, child_hasher);

	std::string_view buffer{data, sizeof(data)};

	child_hashed_writer.write(buffer);

	ASSERT_EQ(sizeof(data), hashed_writer.tellp());
	ASSERT_EQ(sizeof(data), child_hashed_writer.tellp());

	auto calculated_hash       = hasher->get_hash_binary();
	auto child_calculated_hash = child_hasher->get_hash_binary();

	ASSERT_EQ(hash.size(), calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), calculated_hash.data(), hash.size()));

	ASSERT_EQ(hash.size(), child_calculated_hash.size());
	ASSERT_TRUE(0 == memcmp(hash.data(), child_calculated_hash.data(), hash.size()));
}

int main(int argc, char **argv)
{
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}