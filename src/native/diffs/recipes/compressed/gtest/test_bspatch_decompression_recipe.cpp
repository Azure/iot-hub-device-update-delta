/**
 * @file test_bspatch_decompression_recipe.cpp
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

#include <io/buffer/io_device.h>
#include <io/buffer/reader_factory.h>
#include <io/buffer/writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <diffs/recipes/compressed/bspatch_decompression_recipe.h>

#include <diffs/core/kitchen.h>

#include "main.h"

TEST(bspatch_decompression_recipe, make_sequential_reader)
{
	// First, acquire the raw uncompressed blob by using ZSTD to decompress it
	auto compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};
	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->resize(decompression_reader.size());

	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->size()};
	decompression_reader.read(uncompressed_data);

	// Now make the "old" version by modifying the data.

	auto modified_copy_vector = std::make_shared<std::vector<char>>();
	modified_copy_vector->resize(uncompressed_data.size());
	std::memcpy(modified_copy_vector->data(), uncompressed_data.data(), uncompressed_data.size());

	modify_vector(*modified_copy_vector.get(), uncompressed_data.size() - 100, 101, 23, 37);
	auto modified_copy_data = std::span<char>{modified_copy_vector->data(), modified_copy_vector->size()};

	// Now use BSDIFF compressor to create a diff

	// Make readers for "old" and "new" file
	using device    = archive_diff::io::buffer::io_device;
	auto old_reader = device::make_reader(modified_copy_vector, device::size_kind::vector_size);
	auto new_reader = device::make_reader(uncompressed_data_vector, device::size_kind::vector_size);

	// Make a writer for "diff"
	auto diff_data_vector = std::make_shared<std::vector<char>>();
	archive_diff::io::buffer::writer diff_writer(diff_data_vector);
	auto diff_data = std::string_view{diff_data_vector->data(), diff_data_vector->size()};

	// Call into compressor
	archive_diff::io::compressed::bsdiff_compressor::delta_compress(old_reader, new_reader, &diff_writer);

	// Now use BSPATCH decompression reader to apply diff

	// Make reader for "diff"
	auto diff_reader = device::make_reader(diff_data_vector, device::size_kind::vector_size);

	// archive_diff::test_utility::write_to_file(*modified_copy, fs::temp_directory_path() / "old" );
	// archive_diff::test_utility::write_to_file(*uncompressed_data_vector, fs::temp_directory_path() / "new");
	// archive_diff::test_utility::write_to_file(*diff_data_vector, fs::temp_directory_path() / "diff");

	auto uncompressed_item = create_definition_from_span(uncompressed_data);
	auto diff_item         = create_definition_from_data(diff_data);
	auto basis_item        = create_definition_from_span(modified_copy_data);

	std::vector<uint64_t> number_ingredients;
	std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
	item_ingredients.push_back(diff_item);
	item_ingredients.push_back(basis_item);

	archive_diff::diffs::recipes::compressed::bspatch_decompression_recipe::recipe_template recipe_template{};
	auto decompress_recipe = recipe_template.create_recipe(uncompressed_item, number_ingredients, item_ingredients);

	// verify there are two item ingredients
	auto &item_ingredients_found = decompress_recipe->get_item_ingredients();
	ASSERT_EQ(2, item_ingredients_found.size());

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	kitchen->add_recipe(decompress_recipe);
	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> diff_data_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			diff_data_vector, device::size_kind::vector_capacity);

	auto prep_compressed = std::make_shared<archive_diff::diffs::core::prepared_item>(
		diff_item, archive_diff::diffs::core::prepared_item::reader_kind{diff_data_factory});

	kitchen->store_item(prep_compressed);
	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	std::shared_ptr<archive_diff::io::reader_factory> basis_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			modified_copy_vector, device::size_kind::vector_size);
	auto prep_basis = std::make_shared<archive_diff::diffs::core::prepared_item>(
		basis_item, archive_diff::diffs::core::prepared_item::reader_kind{basis_factory});
	kitchen->store_item(prep_basis);
	kitchen->request_item(uncompressed_item);
	ASSERT_TRUE(kitchen->process_requested_items());

	ASSERT_TRUE(kitchen->can_fetch_item(uncompressed_item));

	auto prep_uncompressed = kitchen->fetch_item(uncompressed_item);
	ASSERT_FALSE(prep_uncompressed->can_make_reader());

	auto sequential_reader = prep_uncompressed->make_sequential_reader();

	std::vector<char> uncompressed_from_recipe_vector{};
	uncompressed_from_recipe_vector.resize(sequential_reader->size());
	auto uncompressed_from_recipe_data =
		std::span<char>{uncompressed_from_recipe_vector.data(), uncompressed_from_recipe_vector.size()};

	ASSERT_EQ(sequential_reader->size(), uncompressed_data.size());
	sequential_reader->read_all_remaining(uncompressed_from_recipe_vector);

	ASSERT_EQ(uncompressed_from_recipe_data.size(), uncompressed_data.size());
	ASSERT_EQ(
		0,
		std::memcmp(
			uncompressed_data.data(), uncompressed_from_recipe_data.data(), uncompressed_from_recipe_data.size()));
}
