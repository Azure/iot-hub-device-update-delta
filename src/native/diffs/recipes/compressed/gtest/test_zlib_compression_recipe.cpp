/**
 * @file test_zlib_compression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <language_support/include_filesystem.h>

#include <io/buffer/reader_factory.h>
#include <io/file/io_device.h>

#include <diffs/recipes/compressed/zlib_compression_recipe.h>
#include <diffs/core/prepared_item.h>

#include <diffs/core/kitchen.h>

#include "main.h"

TEST(zlib_compression_recipe, make_sequential_reader)
{
	auto temp_path = fs::temp_directory_path() / "zlib_compression_recipe.make_sequential_reader";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

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

	archive_diff::diffs::recipes::compressed::zlib_compression_recipe::recipe_template recipe_template{};

	auto uncompressed_item = create_definition_from_span(uncompressed_data);
	auto compressed_item   = create_definition_from_span(compressed_data);

	std::vector<uint64_t> number_ingredients;
	number_ingredients.push_back(0); // 0 corresponds to raw/deflate, 1 to gz
	number_ingredients.push_back(9); // compression level
	std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
	item_ingredients.push_back(uncompressed_item);

	auto compress_recipe = recipe_template.create_recipe(compressed_item, number_ingredients, item_ingredients);

	// verify there is one item ingredient
	auto item_ingredients_found = compress_recipe->get_item_ingredients();
	ASSERT_EQ(1, item_ingredients_found.size());

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(compressed_item));

	kitchen->add_recipe(compress_recipe);
	ASSERT_FALSE(kitchen->can_fetch_item(compressed_item));

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> uncompressed_data_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			uncompressed_data_vector, device::size_kind::vector_capacity);

	auto prep_uncompressed = std::make_shared<archive_diff::diffs::core::prepared_item>(
		uncompressed_item, archive_diff::diffs::core::prepared_item::reader_kind{uncompressed_data_factory});

	kitchen->store_item(prep_uncompressed);

	kitchen->request_item(compressed_item);
	ASSERT_TRUE(kitchen->process_requested_items());
	ASSERT_TRUE(kitchen->can_fetch_item(compressed_item));

	auto prep_compressed = kitchen->fetch_item(compressed_item);

	ASSERT_FALSE(prep_compressed->can_make_reader());

	auto sequential_reader = prep_compressed->make_sequential_reader();

	std::vector<char> compressed_from_recipe_vector{};
	compressed_from_recipe_vector.reserve(static_cast<size_t>(sequential_reader->size()));
	auto compressed_from_recipe_data =
		std::span<char>{compressed_from_recipe_vector.data(), compressed_from_recipe_vector.capacity()};
	sequential_reader->read(compressed_from_recipe_data);

	ASSERT_EQ(compressed_from_recipe_data.size(), compressed_data.size());
	ASSERT_EQ(
		0, std::memcmp(compressed_data.data(), compressed_from_recipe_data.data(), compressed_from_recipe_data.size()));
}