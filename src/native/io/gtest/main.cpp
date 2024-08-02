/**
 * @file main.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>

#include <test_utility/gtest_includes.h>
// #include <vector>
// #include <fstream>
// #include <filesystem>
//
// #include <io/file/binary_file_reader.h>
// #include <io/file/binary_file_readerwriter.h>
// #include <io/file/binary_file_writer.h>
//
// #include <io/sequential/basic_reader_wrapper.h>
//
// #include <io/reader_view.h>
//
// namespace fs = std::filesystem;
//
//
// const fs::path c_sample_file_compressed = "sample.zst";
//
// fs::path test_data_root;
//
// fs::path get_data_file(const fs::path file)
//{
//	auto path = test_data_root / file;
//	return path;
// }
//
// bool reader_and_file_are_equal(
//	archive_diff::io::reader &reader, fs::path file_path, uint64_t offset, uint64_t length, size_t chunk_size)
//{
//	std::vector<char> reader_data;
//	std::vector<char> file_data;
//
//	reader_data.reserve(chunk_size);
//	file_data.reserve(chunk_size);
//
//	std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);
//
//	file_stream.seekg(offset);
//
//	auto reader_offset = 0ull;
//	auto remaining     = length;
//
//	while (remaining)
//	{
//		size_t to_read = std::min<size_t>(chunk_size, static_cast<size_t>(remaining));
//
//		file_stream.read(file_data.data(), to_read);
//		auto actual_read = file_stream.gcount();
//
//		if (actual_read == 0)
//		{
//			break;
//		}
//
//		reader.read(reader_offset, std::span<char>{reader_data.data(), static_cast<size_t>(actual_read)});
//
//		if (0 != memcmp(reader_data.data(), file_data.data(), to_read))
//		{
//			return false;
//		}
//
//		remaining -= to_read;
//		reader_offset += to_read;
//	}
//
//	return true;
// }
//
// void compare_reader_and_file(archive_diff::io::reader &reader, fs::path test_file, uint64_t offset, uint64_t length)
//{
//	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 1024));
//	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 32 * 1024));
//	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 64 * 1024));
//	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, 1024 * 1024));
//	EXPECT_TRUE(reader_and_file_are_equal(reader, test_file, offset, length, length));
// }
//
// bool FileAndReaderHaveIdenticalContent(fs::path path, std::shared_ptr<archive_diff::io::reader>& reader, uint64_t
// offset, uint64_t length)
//{
//	auto size = fs::file_size(path);
//
//	std::ifstream file(path, std::ios::in | std::ios::binary);
//
//	file.seekg(offset);
//
//	auto remaining = length;
//
//	std::vector<char> read_buffer1;
//	std::vector<char> read_buffer2;
//
//	const size_t read_block_size = static_cast<size_t>(32 * 1024);
//	read_buffer1.reserve(read_block_size);
//	read_buffer2.reserve(read_block_size);
//
//	archive_diff::io::reader_view reader_with_offset = archive_diff::io::reader_view(reader, offset);
//	archive_diff::io::sequential::basic_reader_wrapper sequential_reader{&reader_with_offset};
//
//	while (remaining)
//	{
//		size_t to_read = std::min(static_cast<size_t>(read_block_size), static_cast<size_t>(remaining));
//
//		file.read(read_buffer1.data(), to_read);
//		sequential_reader.read(std::span<char>{read_buffer2.data(), to_read});
//
//		if (0 != memcmp(read_buffer1.data(), read_buffer2.data(), to_read))
//		{
//			return false;
//		}
//
//		remaining -= to_read;
//	}
//
//	return true;
// }
//

int main(int argc, char **argv)
{
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}