/**
 * @file test_zlib_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <test_utility/gtest_includes.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>

#include <io/compressed/zlib_decompression_reader.h>
#include <io/buffer/io_device.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

TEST(zlib_decompression_reader, compare_against_known_result)
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

	using init_type = archive_diff::io::compressed::zlib_helpers::init_type;
	archive_diff::io::compressed::zlib_decompression_reader decompression_reader(
		compressed_file_reader, uncompressed_size, init_type::raw);

	ASSERT_EQ(uncompressed_size, decompression_reader.size());

	std::vector<char> uncompressed_from_reader_vector{};
	uncompressed_from_reader_vector.reserve(static_cast<size_t>(decompression_reader.size()));
	auto uncompressed_from_reader_data =
		std::span<char>{uncompressed_from_reader_vector.data(), uncompressed_from_reader_vector.capacity()};
	decompression_reader.read(uncompressed_from_reader_data);

	ASSERT_EQ(uncompressed_from_reader_data.size(), uncompressed_data.size());
	ASSERT_EQ(
		0,
		std::memcmp(
			uncompressed_data.data(), uncompressed_from_reader_data.data(), uncompressed_from_reader_data.size()));
}