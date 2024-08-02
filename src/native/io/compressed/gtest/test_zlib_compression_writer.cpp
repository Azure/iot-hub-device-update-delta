/**
 * @file test_zlib_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <test_utility/gtest_includes.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>

#include <io/compressed/zlib_compression_writer.h>
#include <io/compressed/zlib_decompression_reader.h>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

TEST(zlib_compression_writer, compare_against_known_result)
{
	const fs::path compressed_path = g_test_data_root / c_sample_file_deflate_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());
	auto compressed_size        = static_cast<size_t>(compressed_file_reader.size());
	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(compressed_size);
	auto compressed_data = std::span<char>{compressed_data_vector->data(), compressed_size};
	compressed_file_reader.read(0, compressed_data);

	const fs::path uncompressed_path = g_test_data_root / c_sample_file_deflate_uncompressed;

	auto uncompressed_file_reader = archive_diff::io::file::io_device::make_reader(uncompressed_path.string());
	auto uncompressed_size        = static_cast<size_t>(uncompressed_file_reader.size());
	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(uncompressed_size);
	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_size};
	uncompressed_file_reader.read(0, uncompressed_data);

	auto buffer_for_writer = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(buffer_for_writer);
	std::shared_ptr<archive_diff::io::sequential::writer> seq_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	using init_type = archive_diff::io::compressed::zlib_helpers::init_type;
	archive_diff::io::compressed::zlib_compression_writer compression_writer(seq_writer, 9, init_type::raw);

	compression_writer.write(uncompressed_data);
	compression_writer.flush();

	ASSERT_EQ(compressed_size, buffer_for_writer->size());
	ASSERT_EQ(compressed_data.size(), buffer_for_writer->size());

	ASSERT_EQ(0, std::memcmp(compressed_data.data(), buffer_for_writer->data(), buffer_for_writer->size()));
}