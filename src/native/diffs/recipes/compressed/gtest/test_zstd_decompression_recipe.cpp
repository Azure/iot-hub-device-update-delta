/**
 * @file test_zstd_decompression_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <language_support/include_filesystem.h>

#include <io/buffer/reader_factory.h>
#include <io/buffer/writer.h>
#include <io/compressed/zstd_compression_writer.h>
#include <io/compressed/zstd_decompression_reader.h>
#include <io/file/io_device.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <diffs/recipes/basic/slice_recipe.h>
#include <diffs/recipes/compressed/zstd_decompression_recipe.h>

#include <diffs/core/kitchen.h>

#include "main.h"

TEST(zstd_decompression_recipe, make_sequential_reader)
{
	auto temp_path = fs::temp_directory_path() / "recipe_zstd_decompression.make_sequential_reader";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());
	auto compressed_size        = static_cast<size_t>(compressed_file_reader.size());
	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(compressed_size);
	auto compressed_data = std::span<char>{compressed_data_vector->data(), compressed_size};
	compressed_file_reader.read(0, compressed_data);

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));
	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read(uncompressed_data);

	archive_diff::diffs::recipes::compressed::zstd_decompression_recipe::recipe_template recipe_template{};

	auto uncompressed_item = create_definition_from_span(uncompressed_data);
	auto compressed_item   = create_definition_from_span(compressed_data);

	std::vector<uint64_t> number_ingredients;
	std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
	item_ingredients.push_back(compressed_item);

	auto decompress_recipe = recipe_template.create_recipe(uncompressed_item, number_ingredients, item_ingredients);

	// verify there is one item ingredient
	auto item_ingredients_found = decompress_recipe->get_item_ingredients();
	ASSERT_EQ(1, item_ingredients_found.size());

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	kitchen->add_recipe(decompress_recipe);
	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> compressed_data_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			compressed_data_vector, device::size_kind::vector_capacity);
	auto prep_compressed = std::make_shared<archive_diff::diffs::core::prepared_item>(
		compressed_item, archive_diff::diffs::core::prepared_item::reader_kind{compressed_data_factory});

	kitchen->store_item(prep_compressed);

	kitchen->request_item(uncompressed_item);
	ASSERT_TRUE(kitchen->process_requested_items());
	ASSERT_TRUE(kitchen->can_fetch_item(uncompressed_item));

	auto prep_uncompressed = kitchen->fetch_item(uncompressed_item);

	ASSERT_FALSE(prep_uncompressed->can_make_reader());

	auto sequential_reader = prep_uncompressed->make_sequential_reader();

	std::vector<char> uncompressed_from_recipe_vector{};
	uncompressed_from_recipe_vector.reserve(static_cast<size_t>(sequential_reader->size()));
	auto uncompressed_from_recipe_data =
		std::span<char>{uncompressed_from_recipe_vector.data(), uncompressed_from_recipe_vector.capacity()};
	sequential_reader->read(uncompressed_from_recipe_data);

	ASSERT_EQ(uncompressed_from_recipe_data.size(), uncompressed_data.size());
	ASSERT_EQ(
		0,
		std::memcmp(
			uncompressed_data.data(), uncompressed_from_recipe_data.data(), uncompressed_from_recipe_data.size()));
}

TEST(zstd_decompression_recipe, slice)
{
	auto temp_path = fs::temp_directory_path() / "recipe_zstd_decompression.make_sequential_reader";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());
	auto compressed_size        = static_cast<size_t>(compressed_file_reader.size());
	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(compressed_size);
	auto compressed_data = std::span<char>{compressed_data_vector->data(), compressed_size};
	compressed_file_reader.read(0, compressed_data);

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));
	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	decompression_reader.read(uncompressed_data);

	archive_diff::diffs::recipes::compressed::zstd_decompression_recipe::recipe_template recipe_template{};

	auto uncompressed_item = create_definition_from_span(uncompressed_data);
	auto compressed_item   = create_definition_from_span(compressed_data);

	std::vector<uint64_t> number_ingredients;
	std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
	item_ingredients.push_back(compressed_item);

	auto decompress_recipe = recipe_template.create_recipe(uncompressed_item, number_ingredients, item_ingredients);

	// verify there is one item ingredient
	auto item_ingredients_found = decompress_recipe->get_item_ingredients();
	ASSERT_EQ(1, item_ingredients_found.size());

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	kitchen->add_recipe(decompress_recipe);
	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> compressed_data_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			compressed_data_vector, device::size_kind::vector_capacity);
	auto prep_compressed = std::make_shared<archive_diff::diffs::core::prepared_item>(
		compressed_item, archive_diff::diffs::core::prepared_item::reader_kind{compressed_data_factory});

	kitchen->store_item(prep_compressed);

	kitchen->request_item(uncompressed_item);
	ASSERT_TRUE(kitchen->process_requested_items());

	ASSERT_TRUE(kitchen->can_fetch_item(uncompressed_item));

	archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_template;

	const uint64_t c_slice_offset = 100;
	const uint64_t c_slice_length = 1000;
	auto slice_data               = std::string_view{uncompressed_data.data() + c_slice_offset, c_slice_length};
	auto slice_item               = create_definition_from_data(slice_data);

	std::vector<uint64_t> slice_number_ingredients{100};
	std::vector<archive_diff::diffs::core::item_definition> slice_item_ingredients{uncompressed_item};

	auto slice_recipe = slice_template.create_recipe(slice_item, slice_number_ingredients, slice_item_ingredients);

	ASSERT_FALSE(kitchen->can_fetch_item(slice_item));
	kitchen->add_recipe(slice_recipe);

	kitchen->request_item(slice_item);
	ASSERT_TRUE(kitchen->process_requested_items());

	ASSERT_TRUE(kitchen->can_fetch_item(slice_item));

	auto prep_slice = kitchen->fetch_item(slice_item);
	ASSERT_TRUE(prep_slice->can_make_reader());

	bool caught_exception{false};
	try
	{
		auto reader = prep_slice->make_reader();
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::diff_slicing_invalid_state)
		{
			caught_exception = true;
		}
	}
	ASSERT_TRUE(caught_exception);

	kitchen->resume_slicing();

	auto reader = prep_slice->make_reader();
	ASSERT_EQ(reader.size(), slice_data.size());

	std::vector<char> result;
	reader.read_all(result);
	ASSERT_EQ(reader.size(), result.size());
	ASSERT_EQ(0, std::memcmp(result.data(), slice_data.data(), slice_data.size()));

	kitchen->cancel_slicing();
}

TEST(zstd_decompression_recipe, with_basis)
{
	auto temp_path = fs::temp_directory_path() / "recipe_zstd_decompression.with_basis";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = g_test_data_root / c_sample_file_zst_compressed;

	auto compressed_file_reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());
	auto compressed_size        = static_cast<size_t>(compressed_file_reader.size());
	auto compressed_data_vector = std::make_shared<std::vector<char>>();
	compressed_data_vector->reserve(compressed_size);
	auto compressed_data = std::span<char>{compressed_data_vector->data(), compressed_size};
	compressed_file_reader.read(0, compressed_data);

	archive_diff::io::compressed::zstd_decompression_reader decompression_reader{
		compressed_file_reader, c_sample_file_zst_uncompressed_size};

	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);

	auto uncompressed_data_vector = std::make_shared<std::vector<char>>();
	uncompressed_data_vector->reserve(static_cast<size_t>(decompression_reader.size()));
	auto uncompressed_data = std::span<char>{uncompressed_data_vector->data(), uncompressed_data_vector->capacity()};
	ASSERT_EQ(decompression_reader.size(), c_sample_file_zst_uncompressed_size);
	decompression_reader.read(uncompressed_data);

	auto modified_copy_vector = std::make_shared<std::vector<char>>();
	modified_copy_vector->resize(uncompressed_data.size());
	std::memcpy(modified_copy_vector->data(), uncompressed_data.data(), uncompressed_data.size());
	modify_vector(*(modified_copy_vector.get()), uncompressed_data.size() - 100, 101, 23, 37);
	auto modified_copy_data = std::span<char>{modified_copy_vector->data(), modified_copy_vector->size()};

	auto delta_compressed_data_vector = std::make_shared<std::vector<char>>();
	std::shared_ptr<archive_diff::io::writer> buffer_writer =
		std::make_shared<archive_diff::io::buffer::writer>(delta_compressed_data_vector);
	std::shared_ptr<archive_diff::io::sequential::writer> seq_buffer_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(buffer_writer);

	archive_diff::io::compressed::compression_dictionary dictionary{modified_copy_vector};
	archive_diff::io::compressed::zstd_compression_writer compression_writer{
		seq_buffer_writer,
		c_sample_file_zst_compression_level,
		c_sample_file_zst_uncompressed_size,
		std::move(dictionary)};
	compression_writer.write(uncompressed_data);

	const size_t c_delta_compressed_file_size = 28019; // determined with previous runs
	ASSERT_EQ(delta_compressed_data_vector->size(), c_delta_compressed_file_size);

	auto delta_compressed_data =
		std::span<char>{delta_compressed_data_vector->data(), delta_compressed_data_vector->size()};

	auto uncompressed_item     = create_definition_from_span(uncompressed_data);
	auto delta_compressed_item = create_definition_from_span(delta_compressed_data);
	auto basis_item            = create_definition_from_span(modified_copy_data);

	std::vector<uint64_t> number_ingredients;
	std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
	item_ingredients.push_back(delta_compressed_item);
	item_ingredients.push_back(basis_item);

	archive_diff::diffs::recipes::compressed::zstd_decompression_recipe::recipe_template recipe_template{};
	auto decompress_recipe = recipe_template.create_recipe(uncompressed_item, number_ingredients, item_ingredients);

	// verify there are two item ingredients
	auto item_ingredients_found = decompress_recipe->get_item_ingredients();
	ASSERT_EQ(2, item_ingredients_found.size());

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	kitchen->add_recipe(decompress_recipe);
	ASSERT_FALSE(kitchen->can_fetch_item(uncompressed_item));

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> delta_compressed_data_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(
			delta_compressed_data_vector, device::size_kind::vector_capacity);
	auto prep_compressed = std::make_shared<archive_diff::diffs::core::prepared_item>(
		delta_compressed_item, archive_diff::diffs::core::prepared_item::reader_kind{delta_compressed_data_factory});

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
	uncompressed_from_recipe_vector.resize(static_cast<size_t>(sequential_reader->size()));
	auto uncompressed_from_recipe_data =
		std::span<char>{uncompressed_from_recipe_vector.data(), uncompressed_from_recipe_vector.size()};

	sequential_reader->read_all_remaining(uncompressed_from_recipe_vector);

	ASSERT_EQ(uncompressed_from_recipe_data.size(), uncompressed_data.size());
	ASSERT_EQ(
		0,
		std::memcmp(
			uncompressed_data.data(), uncompressed_from_recipe_data.data(), uncompressed_from_recipe_data.size()));
}
