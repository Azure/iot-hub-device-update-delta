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

TEST(zstd_decompression_writer, and_then_compress_with_reader)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;
	auto compressed_size = fs::file_size(compressed_path);

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

	auto uncompressed_data_vector2 = std::make_shared<std::vector<char>>();
	uncompressed_data_vector2->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->size()};

	auto uncompressed_data2 = std::span<char>{uncompressed_data_vector2->data(), uncompressed_data_vector2->capacity()};
	decompression_reader.read(uncompressed_data2);

	ASSERT_EQ(uncompressed_data.size(), uncompressed_data2.size());

	ASSERT_EQ(0, std::memcmp(uncompressed_data.data(), uncompressed_data2.data(), uncompressed_data.size()));

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_size);

	archive_diff::io::compressed::zstd_compression_reader compression_reader{
		uncompressed_reader, c_sample_file_zst_compression_level, c_sample_file_zst_uncompressed_size, compressed_size};

	ASSERT_EQ(compression_reader.size(), compressed_size);

	std::vector<char> compressed_data_vector;
	compressed_data_vector.reserve(static_cast<size_t>(compression_reader.size()));
	auto compressed_data = std::span<char>{compressed_data_vector.data(), compressed_data_vector.capacity()};
	compression_reader.read(compressed_data);

	std::vector<char> compressed_from_file_data_vector;
	compressed_from_file_data_vector.reserve(static_cast<size_t>(compressed_file_reader.size()));
	auto compressed_from_file_data =
		std::span<char>{compressed_from_file_data_vector.data(), compressed_from_file_data_vector.capacity()};
	compressed_file_reader.read(0, compressed_from_file_data);

	ASSERT_EQ(compressed_from_file_data.size(), compressed_data.size());
	ASSERT_EQ(compressed_from_file_data.size(), compressed_size);
}
