/**
 * @file test_all_zeros_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <io/reader.h>
#include <io/sequential/reader.h>

#include <hashing/hasher.h>

#include <diffs/recipes/basic/all_zeros_recipe.h>
#include <io/all_zeros_io_device.h>

#include <diffs/core/kitchen.h>

void test_all_zero_reader(
	archive_diff::io::reader &reader,
	size_t num_zeros,
	const archive_diff::hashing::hash &expected_hash,
	std::string_view expected_data)
{
	std::vector<char> result_data;
	// fill with something besides zeros to not trivially succeed
	result_data.resize(num_zeros, 0xa);

	reader.read(0, result_data);

	ASSERT_EQ(result_data.size(), expected_data.size());
	ASSERT_EQ(0, std::memcmp(result_data.data(), expected_data.data(), expected_data.size()));

	archive_diff::hashing::hasher result_hasher(archive_diff::hashing::algorithm::sha256);
	result_hasher.hash_data(result_data);
	auto actual_hash = result_hasher.get_hash();

	archive_diff::hashing::hash::verify_hashes_match(expected_hash, actual_hash);
}

void test_all_zero_sequential_reader(
	archive_diff::io::sequential::reader *reader,
	size_t num_zeros,
	const archive_diff::hashing::hash &expected_hash,
	std::string_view expected_data)
{
	std::vector<char> result_data;
	// fill with something besides zeros to not trivially succeed
	result_data.resize(num_zeros, 0xa);

	reader->read(result_data);

	ASSERT_EQ(result_data.size(), expected_data.size());
	ASSERT_EQ(0, std::memcmp(result_data.data(), expected_data.data(), expected_data.size()));

	archive_diff::hashing::hasher result_hasher(archive_diff::hashing::algorithm::sha256);
	result_hasher.hash_data(result_data);
	auto actual_hash = result_hasher.get_hash();

	archive_diff::hashing::hash::verify_hashes_match(expected_hash, actual_hash);
}

TEST(zeros_io_device, make_reader)
{
	for (size_t num_zeros = 100; num_zeros <= 10000; num_zeros += 100)
	{
		std::vector<char> expected_data;
		expected_data.resize(num_zeros);
		archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
		hasher.hash_data(expected_data);
		auto expected_hash = hasher.get_hash();

		auto reader = archive_diff::io::all_zeros_io_device::make_reader(num_zeros);

		test_all_zero_reader(reader, num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});
	}
}

TEST(all_zero_recipe, prepare)
{
	for (size_t num_zeros = 100; num_zeros <= 10000; num_zeros += 100)
	{
		std::vector<char> expected_data;
		expected_data.resize(num_zeros);
		archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
		hasher.hash_data(expected_data);
		auto expected_hash = hasher.get_hash();

		auto item = archive_diff::diffs::core::item_definition{num_zeros}.with_hash(expected_hash);

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(num_zeros);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		archive_diff::diffs::recipes::basic::all_zeros_recipe recipe(item, number_ingredients, item_ingredients);

		// verify there are no item ingredients
		auto item_ingredients_found = recipe.get_item_ingredients();
		ASSERT_EQ(0, item_ingredients_found.size());

		// prepare the recipe
		auto kitchen = archive_diff::diffs::core::kitchen::create();
		std::vector<std::shared_ptr<archive_diff::diffs::core::prepared_item>> no_prepared_ingredients;
		auto prepared_item = recipe.prepare(kitchen.get(), no_prepared_ingredients);

		ASSERT_EQ(true, prepared_item->can_make_reader());

		auto reader = prepared_item->make_reader();
		test_all_zero_reader(reader, num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});

		auto sequential_reader = prepared_item->make_sequential_reader();
		test_all_zero_sequential_reader(
			sequential_reader.get(), num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});
	}
}

TEST(all_zero_recipe_template, make_reader)
{
	for (size_t num_zeros = 100; num_zeros <= 10000; num_zeros += 100)
	{
		std::vector<char> expected_data;
		expected_data.resize(num_zeros);
		archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
		hasher.hash_data(expected_data);
		auto expected_hash = hasher.get_hash();

		auto item = archive_diff::diffs::core::item_definition{num_zeros}.with_hash(expected_hash);

		archive_diff::diffs::recipes::basic::all_zeros_recipe::recipe_template recipe_template{};

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(num_zeros);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		auto recipe = recipe_template.create_recipe(item, number_ingredients, item_ingredients);

		// verify there are no item ingredients
		auto item_ingredients_found = recipe->get_item_ingredients();
		ASSERT_EQ(0, item_ingredients_found.size());

		// prepare the recipe
		auto kitchen = archive_diff::diffs::core::kitchen::create();

		std::vector<std::shared_ptr<archive_diff::diffs::core::prepared_item>> no_prepared_ingredients;
		auto prepared_item = recipe->prepare(kitchen.get(), no_prepared_ingredients);

		ASSERT_EQ(true, prepared_item->can_make_reader());

		auto reader = prepared_item->make_reader();
		test_all_zero_reader(reader, num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});

		auto sequential_reader = prepared_item->make_sequential_reader();
		test_all_zero_sequential_reader(
			sequential_reader.get(), num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});
	}
}

TEST(all_zero_recipe, fetch_from_kitchen)
{
	for (size_t num_zeros = 100; num_zeros <= 10000; num_zeros += 100)
	{
		std::vector<char> expected_data;
		expected_data.resize(num_zeros);
		archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
		hasher.hash_data(expected_data);
		auto expected_hash = hasher.get_hash();

		auto item = archive_diff::diffs::core::item_definition{num_zeros}.with_hash(expected_hash);

		archive_diff::diffs::recipes::basic::all_zeros_recipe::recipe_template recipe_template{};

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(num_zeros);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		auto recipe = recipe_template.create_recipe(item, number_ingredients, item_ingredients);

		// verify there are no item ingredients
		auto item_ingredients_found = recipe->get_item_ingredients();
		ASSERT_EQ(0, item_ingredients_found.size());

		auto kitchen = archive_diff::diffs::core::kitchen::create();

		kitchen->add_recipe(recipe);
		ASSERT_FALSE(kitchen->can_fetch_item(item));

		// request the item
		kitchen->request_item(item);
		kitchen->process_requested_items();

		ASSERT_TRUE(kitchen->can_fetch_item(item));

		auto prepared_item = kitchen->fetch_item(item);
		ASSERT_EQ(true, prepared_item->can_make_reader());

		auto reader = prepared_item->make_reader();
		test_all_zero_reader(reader, num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});

		auto sequential_reader = prepared_item->make_sequential_reader();
		test_all_zero_sequential_reader(
			sequential_reader.get(), num_zeros, expected_hash, std::string_view{expected_data.data(), num_zeros});
	}
}
