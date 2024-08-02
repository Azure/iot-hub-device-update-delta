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

TEST(zstd_decompression_reader, and_then_compress_with_reader)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

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

	uint64_t compressed_size = fs::file_size(compressed_path);
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

	ASSERT_EQ(0, memcmp(compressed_from_file_data.data(), compressed_data.data(), compressed_data.size()));
}

TEST(zstd_decompression_reader, and_compress_with_basis_then_decompress)
{
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read(uncompressed_data);

	auto modified_copy = std::make_shared<std::vector<char>>();
	modified_copy->resize(uncompressed_data.size());
	std::memcpy(modified_copy->data(), uncompressed_data.data(), uncompressed_data.size());

	modify_vector(*modified_copy.get(), uncompressed_data.size() - 100, 101, 23, 37);

	auto delta_compressed_data_vector = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(delta_compressed_data_vector);
	std::shared_ptr<archive_diff::io::sequential::writer> seq_buffer_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	archive_diff::io::compressed::zstd_compression_writer compression_writer{
		seq_buffer_writer, c_sample_file_zst_compression_level, c_sample_file_zst_uncompressed_size, modified_copy};
	compression_writer.write(uncompressed_data);

	const size_t c_delta_compressed_file_size = 28019; // determined with previous runs
	ASSERT_EQ(delta_compressed_data_vector->size(), c_delta_compressed_file_size);

	using device                 = archive_diff::io::buffer::io_device;
	auto delta_compressed_reader = device::make_reader(delta_compressed_data_vector, device::size_kind::vector_size);

	auto delta_uncompressed_data_vector = std::make_shared<std::vector<char>>();
	delta_uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));

	auto delta_uncompressed_data =
		std::span<char>{delta_uncompressed_data_vector->data(), delta_uncompressed_data_vector->capacity()};

	// first try without dictionary
	{
		archive_diff::io::compressed::zstd_decompression_reader delta_decompression_reader{
			delta_compressed_reader, c_sample_file_zst_uncompressed_size};

		ASSERT_EQ(delta_decompression_reader.size(), c_sample_file_zst_uncompressed_size);
		bool caught_exception{false};
		try
		{
			delta_decompression_reader.read(delta_uncompressed_data);
		}
		catch (archive_diff::errors::user_exception &)
		{
			caught_exception = true;
		}
		ASSERT_TRUE(caught_exception);
	}

	archive_diff::io::compressed::zstd_decompression_reader delta_decompression_reader{
		delta_compressed_reader, c_sample_file_zst_uncompressed_size, modified_copy};

	ASSERT_EQ(delta_decompression_reader.size(), c_sample_file_zst_uncompressed_size);
	delta_decompression_reader.read(delta_uncompressed_data);

	ASSERT_EQ(delta_uncompressed_data.size(), uncompressed_data.size());
	ASSERT_EQ(0, std::memcmp(delta_uncompressed_data.data(), uncompressed_data.data(), uncompressed_data.size()));
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
