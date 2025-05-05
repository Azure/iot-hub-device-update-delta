/**
 * @file test_zstd_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/file/io_device.h>

#include <io/sequential/basic_writer_wrapper.h>
#include <io/compressed/zstd_decompression_reader.h>
#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>

#include <io/compressed/zstd_compression_writer.h>

#include <language_support/include_filesystem.h>

#include "main.h"
#include "common.h"

TEST(zstd_compression_writer, uncompressed_size_one_byte_too_few)
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
	decompression_reader.read(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(static_cast<size_t>(compressed_size));

	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(compressed_data_vector);
	std::shared_ptr<archive_diff::io::sequential::writer> sequential_buffer_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size + 1;

	archive_diff::io::compressed::zstd_compression_writer compression_writer(
		sequential_buffer_writer, c_sample_file_zst_compression_level, bad_uncompressed_size);

	compression_writer.write(uncompressed_reader);

	auto compressed_data = std::span<char>{compressed_data_vector->data(), compressed_data_vector->capacity()};
	std::memset(compressed_data.data(), 0, compressed_data.size());

	auto file_data_vector = reader_to_vector(compressed_file_reader);
	auto file_data        = std::span<char>{file_data_vector.data(), file_data_vector.capacity()};

	ASSERT_EQ(file_data.size(), compressed_data.size());

	ASSERT_NE(0, std::memcmp(file_data.data(), compressed_data.data(), file_data.size()));

	using device                = archive_diff::io::buffer::io_device;
	auto compressed_data_reader = device::make_reader(compressed_data_vector, device::size_kind::vector_capacity);
	archive_diff::io::compressed::zstd_decompression_reader decompression_reader2{
		compressed_data_reader, c_sample_file_zst_uncompressed_size};
	ASSERT_EQ(decompression_reader2.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector2 = std::make_shared<std::vector<char>>();
	uncompressed_data_vector2->reserve(static_cast<size_t>(decompression_reader2.size()));

	auto uncompressed_data2 = std::span<char>{uncompressed_data_vector2->data(), uncompressed_data_vector2->capacity()};

	bool caught_exception{false};
	try
	{
		decompression_reader2.read(uncompressed_data2);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_decompress_stream_failed)
		{
			caught_exception = true;
		}
	}

	ASSERT_TRUE(caught_exception);
}

TEST(zstd_compression_writer, uncompressed_size_one_byte_too_many)
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
	decompression_reader.read(uncompressed_data);

	using device             = archive_diff::io::buffer::io_device;
	auto uncompressed_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_capacity);

	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(static_cast<size_t>(compressed_size));

	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(compressed_data_vector);
	std::shared_ptr<archive_diff::io::sequential::writer> sequential_buffer_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	auto bad_uncompressed_size = c_sample_file_zst_uncompressed_size - 1;

	archive_diff::io::compressed::zstd_compression_writer compression_writer(
		sequential_buffer_writer, c_sample_file_zst_compression_level, bad_uncompressed_size);

	bool caught_exception{false};
	try
	{
		compression_writer.write(uncompressed_reader);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::io_zstd_too_much_data_processed)
		{
			caught_exception = true;
		}
	}
	ASSERT_TRUE(caught_exception);
}