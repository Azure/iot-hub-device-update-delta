/**
 * @file test_zstd_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>

#include <io/compressed/zstd_compression_reader.h>
#include <io/compressed/zstd_compression_writer.h>
#include <io/compressed/zstd_decompression_reader.h>
#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <language_support/include_filesystem.h>

#include "main.h"

TEST(zstd_decompression_reader, against_known_result)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));
	auto uncompressed_data_from_reader =
		std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read(uncompressed_data_from_reader);

	auto uncompressed_path        = g_test_data_root / c_sample_file_zst_uncompressed;
	auto uncompressed_file_reader = archive_diff::io::file::io_device::make_reader(uncompressed_path.string());

	std::vector<char> uncompressed_data_from_file;
	uncompressed_file_reader.read_all(uncompressed_data_from_file);

	ASSERT_EQ(uncompressed_data_from_file.size(), uncompressed_data_from_reader.size());

	ASSERT_EQ(
		0,
		memcmp(
			uncompressed_data_from_file.data(),
			uncompressed_data_from_reader.data(),
			uncompressed_data_from_reader.size()));
}

TEST(zstd_decompression_reader, uncompressed_size_one_byte_too_few)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size - 1;

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, bad_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), bad_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};

	decompression_reader.read(uncompressed_data);
}

TEST(zstd_decompression_reader, uncompressed_size_one_byte_too_many)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size + 1;

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, bad_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), bad_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};

	bool caught_exception{false};
	try
	{
		decompression_reader.read(uncompressed_data);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_decompress_cannot_finish)
		{
			caught_exception = true;
		}
	}

	ASSERT_EQ(caught_exception, true);
}
