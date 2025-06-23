/**
 * @file test_zstd_decompression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/file/io_device.h>

#include <io/compressed/zstd_compression_reader.h>
#include <io/compressed/zstd_decompression_reader.h>
#include <io/compressed/zstd_decompression_writer.h>

#include <io/buffer/writer.h>
#include <io/buffer/io_device.h>

#include <io/sequential/basic_writer_wrapper.h>

#include <language_support/include_filesystem.h>

#include "main.h"

TEST(zstd_decompression_writer, against_known_result)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(uncompressed_data_vector);
	std::shared_ptr<archive_diff::io::sequential::writer> sequential_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	archive_diff::io::compressed::zstd_decompression_writer decompression_writer{sequential_writer};

	decompression_writer.write(compressed_file_reader);

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_path        = g_test_data_root / c_sample_file_zst_uncompressed;
	auto uncompressed_file_reader = archive_diff::io::file::io_device::make_reader(uncompressed_path.string());

	std::vector<char> uncompressed_data_from_file;
	uncompressed_file_reader.read_all(uncompressed_data_from_file);

	ASSERT_EQ(uncompressed_data_from_file.size(), uncompressed_data_vector->size());

	ASSERT_EQ(
		0,
		memcmp(uncompressed_data_from_file.data(), uncompressed_data_vector->data(), uncompressed_data_vector->size()));
}