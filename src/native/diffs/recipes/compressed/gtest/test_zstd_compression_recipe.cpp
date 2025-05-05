/**
 * @file test_zstd_compression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <language_support/include_filesystem.h>

#include <io/file/io_device.h>

#include <diffs/recipes/compressed/zstd_compression_recipe.h>
#include <diffs/core/item_definition_helpers.h>

#include <diffs/core/kitchen.h>

#include "main.h"

TEST(zstd_compression_recipe, create_and_throw)
{
	using namespace archive_diff;
	using namespace archive_diff::diffs::core;
	using namespace archive_diff::diffs::recipes::compressed;
	using namespace archive_diff::io;

	auto temp_path = fs::temp_directory_path() / "recipe_zstd_decompression.make_sequential_reader";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path   = g_test_data_root / c_sample_file_zst_compressed;
	const fs::path uncompressed_path = g_test_data_root / c_sample_file_zst_uncompressed;

	auto uncompressed_file_reader = file::io_device::make_reader(uncompressed_path.string());
	auto compressed_file_reader   = file::io_device::make_reader(compressed_path.string());

	auto uncompressed_item = create_definition_from_reader(uncompressed_file_reader);
	auto compressed_item   = create_definition_from_reader(compressed_file_reader);

	bool caught_exception{false};
	try
	{
		zstd_compression_recipe compressor{
			compressed_item, std::vector<uint64_t>{}, std::vector<item_definition>{uncompressed_item}};
	}
	catch (errors::user_exception &e)
	{
		if (e.get_error() == errors::error_code::recipe_zstd_compression_not_supported)
		{
			caught_exception = true;
		}
	}
	ASSERT_TRUE(caught_exception);
}