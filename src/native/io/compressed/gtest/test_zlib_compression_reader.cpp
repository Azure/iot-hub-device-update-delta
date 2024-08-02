/**
 * @file test_zlib_compression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <test_utility/gtest_includes.h>

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

TEST(zlib_compression_reader, compare_against_known_result)
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
	archive_diff::io::compressed::zlib_compression_reader compression_reader{
		uncompressed_file_reader, 9, uncompressed_size, compressed_size, init_type::raw};

	std::vector<char> from_reader_vector;
	compression_reader.read_all_remaining(from_reader_vector);

	ASSERT_EQ(compressed_size, from_reader_vector.size());
	ASSERT_EQ(0, std::memcmp(compressed_data.data(), from_reader_vector.data(), compressed_size));

	//
	// Now read from a new reader in 3333 byte chunks
	//
	archive_diff::io::compressed::zlib_compression_reader compression_reader2{
		uncompressed_file_reader, 9, uncompressed_size, compressed_size, init_type::raw};

	std::vector<char> from_reader_vector2;
	from_reader_vector2.reserve(compressed_size);

	const size_t block_size = 3333;
	uint64_t offset         = 0;
	uint64_t remaining      = compressed_size;
	char *output_buffer     = const_cast<char *>(from_reader_vector2.data());

	while (remaining > 0)
	{
		auto to_read = std::min<size_t>(block_size, remaining);
		auto span    = std::span<char>{output_buffer + offset, to_read};

		auto actual_read = compression_reader2.read_some(span);

		offset += actual_read;
		remaining -= actual_read;
	}

	ASSERT_EQ(0, std::memcmp(compressed_data.data(), from_reader_vector2.data(), compressed_size));
}