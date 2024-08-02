/**
 * @file test_bspatch_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>
#include <test_utility/dump_helpers.h>

#include <io/file/io_device.h>

// To access the uncompressed sample.zst - we aren't interested
// in testing ZSTD here, but we rely on it to work to test bsdiff
#include <io/compressed/zstd_decompression_reader.h>

#include <io/compressed/bsdiff_compressor.h>
#include <io/compressed/bspatch_decompression_reader.h>

#include <io/buffer/io_device.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include "main.h"

TEST(bspatch_decompression_reader, compress_with_basis_and_then_decompress)
{
	// First, acquire the raw uncompressed blob by using ZSTD to decompress it
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};
	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->resize(static_cast<size_t>(decompression_reader.size()));

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->size()};
	decompression_reader.read(uncompressed_data);

	// Now make the "old" version by modifying the data.

	auto modified_copy = std::make_shared<std::vector<char>>();
	modified_copy->resize(uncompressed_data.size());
	std::memcpy(modified_copy->data(), uncompressed_data.data(), uncompressed_data.size());

	modify_vector(*modified_copy.get(), uncompressed_data.size() - 100, 101, 23, 37);

	// Now use BSDIFF compressor to create a diff

	// Make readers for "old" and "new" file
	using device = archive_diff::io::buffer::io_device;

	auto old_reader = device::make_reader(modified_copy, device::size_kind::vector_size);
	auto new_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_size);

	// Make a writer for "diff"
	auto diff_data_vector = std::make_shared<std::vector<char>>();
	archive_diff::io::buffer::writer diff_writer(diff_data_vector);

	// Call into compressor
	archive_diff::io::compressed::bsdiff_compressor::delta_compress(old_reader, new_reader, &diff_writer);

	// Now use BSPATCH decompression reader to apply diff

	// Make reader for "diff"
	auto diff_reader = device::make_reader(diff_data_vector, device::size_kind::vector_size);

	// archive_diff::test_utility::write_to_file(*modified_copy, fs::temp_directory_path() / "old" );
	// archive_diff::test_utility::write_to_file(*uncompressed_data_vector, fs::temp_directory_path() / "new");
	// archive_diff::test_utility::write_to_file(*diff_data_vector, fs::temp_directory_path() / "diff");

	// Create and read all data from decompression reader
	auto uncompressed_size = uncompressed_data.size();
	archive_diff::io::compressed::bspatch_decompression_reader bspath_decompression_reader(
		diff_reader, uncompressed_size, old_reader);
	std::vector<char> applied_diff_vector;
	bspath_decompression_reader.read_all_remaining(applied_diff_vector);

	ASSERT_EQ(bspath_decompression_reader.size(), uncompressed_size);
	ASSERT_EQ(applied_diff_vector.size(), uncompressed_data.size());
	ASSERT_EQ(0, std::memcmp(applied_diff_vector.data(), uncompressed_data.data(), uncompressed_data.size()));
}
