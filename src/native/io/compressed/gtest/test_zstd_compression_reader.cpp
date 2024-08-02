/**
 * @file test_zstd_compression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <hashing/hasher.h>
#include <io/file/io_device.h>

#include <io/compressed/zstd_compression_reader.h>
#include <io/compressed/zstd_decompression_reader.h>
#include <io/buffer/io_device.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

TEST(zstd_compression_reader, compressed_size_one_byte_too_few)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;
	auto compressed_size = fs::file_size(compressed_path);

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read_some(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto bad_compressed_size = compressed_size - 1;
	archive_diff::io::compressed::zstd_compression_reader compression_reader{
		uncompressed_reader,
		c_sample_file_zst_compression_level,
		c_sample_file_zst_uncompressed_size,
		bad_compressed_size};
	ASSERT_EQ(compression_reader.size(), bad_compressed_size);

	std::vector<char> compressed_data_vector;
	compressed_data_vector.reserve(static_cast<size_t>(compression_reader.size()));
	auto compressed_data_bad_size = std::span<char>{compressed_data_vector.data(), compressed_data_vector.capacity()};

	bool caught_exception{false};
	try
	{
		auto actual_read = compression_reader.read_some(compressed_data_bad_size);
		ASSERT_EQ(actual_read, compressed_size);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_compress_finished_early)
		{
			caught_exception = true;
		}
	}
	ASSERT_EQ(caught_exception, true);
}

TEST(zstd_compression_reader, compressed_size_one_byte_too_many)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;
	auto compressed_size = fs::file_size(compressed_path);

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read_some(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto bad_compressed_size = compressed_size + 1;
	archive_diff::io::compressed::zstd_compression_reader compression_reader{
		uncompressed_reader,
		c_sample_file_zst_compression_level,
		c_sample_file_zst_uncompressed_size,
		bad_compressed_size};
	ASSERT_EQ(compression_reader.size(), bad_compressed_size);

	std::vector<char> compressed_data_vector;
	compressed_data_vector.reserve(static_cast<size_t>(compression_reader.size()));
	auto compressed_data_bad_size = std::span<char>{compressed_data_vector.data(), compressed_data_vector.capacity()};
	auto actual_read              = compression_reader.read_some(compressed_data_bad_size);
	ASSERT_EQ(actual_read, compressed_size);

	bool caught_exception{false};

	char last_byte;
	try
	{
		compression_reader.read(&last_byte);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_compress_stream_failed)
		{
			caught_exception = true;
		}
	}
	ASSERT_EQ(caught_exception, true);

	auto file_data_vector = reader_to_vector(compressed_file_reader);
	auto file_data        = std::span<char>{file_data_vector.data(), file_data_vector.capacity()};

	ASSERT_EQ(file_data.size() + 1, compressed_data_bad_size.size());
	ASSERT_EQ(0, std::memcmp(file_data.data(), compressed_data_bad_size.data(), file_data.size()));
}

TEST(zstd_compression_reader, uncompressed_size_one_byte_too_few)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;
	auto compressed_size = fs::file_size(compressed_path);

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read_some(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size - 1;
	archive_diff::io::compressed::zstd_compression_reader compression_reader{
		uncompressed_reader, c_sample_file_zst_compression_level, bad_uncompressed_size, compressed_size};
	ASSERT_EQ(compression_reader.size(), compressed_size);

	std::vector<char> compressed_data_vector;
	compressed_data_vector.reserve(static_cast<size_t>(compression_reader.size()));
	auto compressed_data = std::span<char>{compressed_data_vector.data(), compressed_data_vector.capacity()};

	bool caught_exception{false};
	try
	{
		compression_reader.read_some(compressed_data);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_compress_cannot_finish)
		{
			caught_exception = true;
		}
	}
	ASSERT_EQ(caught_exception, true);
}

TEST(zstd_compression_reader, uncompressed_size_one_byte_too_many)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;
	auto compressed_size = fs::file_size(compressed_path);

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read_some(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size + 1;
	archive_diff::io::compressed::zstd_compression_reader compression_reader{
		uncompressed_reader, c_sample_file_zst_compression_level, bad_uncompressed_size, compressed_size};
	ASSERT_EQ(compression_reader.size(), compressed_size);

	std::vector<char> compressed_data_vector;
	compressed_data_vector.reserve(static_cast<size_t>(compression_reader.size()));
	auto compressed_data = std::span<char>{compressed_data_vector.data(), compressed_data_vector.capacity()};

	bool caught_exception{false};
	try
	{
		compression_reader.read_some(compressed_data);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_compress_cannot_finish)
		{
			caught_exception = true;
		}
	}
	ASSERT_EQ(caught_exception, true);
}