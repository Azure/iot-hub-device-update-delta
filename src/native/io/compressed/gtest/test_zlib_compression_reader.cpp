/**
 * @file test_zlib_compression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/read_test_file.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>

#include <io/file/binary_file_writer.h>

#include <io/compressed/zlib_compression_reader.h>
#include <io/compressed/zlib_decompression_reader.h>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

using init_type = archive_diff::io::compressed::zlib_helpers::init_type;

std::vector<char> compress_file_using_reader(
	fs::path uncompressed_file, size_t known_compressed_file_size, init_type init);
std::vector<char> compress_file_in_blocks_using_reader(
	fs::path uncompressed_file, size_t known_compressed_file_size, init_type init);

void test_zlib_compression_reader(init_type init, fs::path known_compressed_file)
{
	const fs::path uncompressed_path = g_test_data_root / c_sample_file_zlib_uncompressed;

	auto known_compressed_data = read_test_file(known_compressed_file);
	auto recompressed_data     = compress_file_using_reader(uncompressed_path, known_compressed_data.size(), init);

	ASSERT_EQ(known_compressed_data.size(), recompressed_data.size());
	ASSERT_EQ(0, std::memcmp(known_compressed_data.data(), recompressed_data.data(), known_compressed_data.size()));

	auto recompressed_in_blocks_data =
		compress_file_in_blocks_using_reader(uncompressed_path, known_compressed_data.size(), init);

	ASSERT_EQ(known_compressed_data.size(), recompressed_in_blocks_data.size());
	ASSERT_EQ(
		0, std::memcmp(known_compressed_data.data(), recompressed_in_blocks_data.data(), known_compressed_data.size()));
}

std::vector<char> compress_file_using_reader(
	fs::path uncompressed_file, size_t known_compressed_file_size, init_type init)
{
	auto uncompressed_file_reader = archive_diff::io::file::io_device::make_reader(uncompressed_file.string());
	auto uncompressed_size        = static_cast<size_t>(uncompressed_file_reader.size());

	archive_diff::io::compressed::zlib_compression_reader compression_reader{
		uncompressed_file_reader, Z_BEST_COMPRESSION, uncompressed_size, known_compressed_file_size, init};

	std::vector<char> data;
	compression_reader.read_all_remaining(data);

	return data;
}

std::vector<char> compress_file_in_blocks_using_reader(
	fs::path uncompressed_file, size_t known_compressed_file_size, init_type init)
{
	auto uncompressed_file_reader = archive_diff::io::file::io_device::make_reader(uncompressed_file.string());
	auto uncompressed_size        = static_cast<size_t>(uncompressed_file_reader.size());

	archive_diff::io::compressed::zlib_compression_reader compression_reader{
		uncompressed_file_reader, Z_BEST_COMPRESSION, uncompressed_size, known_compressed_file_size, init};

	std::vector<char> data;
	data.resize(known_compressed_file_size);

	const size_t block_size = 3333;
	uint64_t offset         = 0;
	uint64_t remaining      = known_compressed_file_size;
	char *output_buffer     = const_cast<char *>(data.data());

	while (remaining > 0)
	{
		auto to_read = std::min<size_t>(block_size, remaining);
		auto span    = std::span<char>{output_buffer + offset, to_read};

		auto actual_read = compression_reader.read_some(span);

		offset += actual_read;
		remaining -= actual_read;
	}

	return data;
}

TEST(zlib_compression_reader_raw_deflate, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_deflate_compressed;

	test_zlib_compression_reader(init_type::raw, compressed_path);
}

TEST(zlib_compression_reader_gz, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_gz_compressed;

	test_zlib_compression_reader(init_type::gz, compressed_path);
}

TEST(zlib_compression_reader_zlib, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_zlib_compressed;

	test_zlib_compression_reader(init_type::zlib, compressed_path);
}