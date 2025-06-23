/**
 * @file test_zlib_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/read_test_file.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>

#include <io/compressed/zlib_compression_writer.h>
#include <io/compressed/zlib_decompression_reader.h>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

using init_type = archive_diff::io::compressed::zlib_helpers::init_type;

void test_zlib_compression_writer(init_type init, fs::path compressed_path)
{
	const fs::path uncompressed_path = g_test_data_root / c_sample_file_zlib_uncompressed;
	auto uncompressed_data           = read_test_file(uncompressed_path.string());

	auto buffer_for_writer = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(buffer_for_writer);
	std::shared_ptr<archive_diff::io::sequential::writer> seq_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	archive_diff::io::compressed::zlib_compression_writer compression_writer(seq_writer, Z_BEST_COMPRESSION, init);

	compression_writer.write(uncompressed_data);
	compression_writer.flush();

	auto compressed_data = read_test_file(compressed_path.string());
	ASSERT_EQ(compressed_data.size(), buffer_for_writer->size());
	ASSERT_EQ(0, std::memcmp(compressed_data.data(), buffer_for_writer->data(), buffer_for_writer->size()));
}

TEST(zlib_compression_writer_raw_deflate, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_deflate_compressed;

	test_zlib_compression_writer(init_type::raw, compressed_path);
}

TEST(zlib_compression_writer_gz, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_gz_compressed;

	test_zlib_compression_writer(init_type::gz, compressed_path);
}

TEST(zlib_compression_writer_zlib, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_zlib_compressed;

	test_zlib_compression_writer(init_type::zlib, compressed_path);
}