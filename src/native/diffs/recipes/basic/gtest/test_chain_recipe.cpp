/**
 * @file test_chain_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <hashing/hasher.h>

#include <io/buffer/reader_factory.h>

#include <diffs/recipes/basic/chain_recipe.h>

#include <diffs/core/kitchen.h>

extern const char c_alphabet[]            = "abcdefghijklmnopqrstuvwxyz";
extern const size_t c_letters_in_alphabet = sizeof(c_alphabet) - 1;

using item_definition = archive_diff::diffs::core::item_definition;

TEST(chain_recipe, reader_from_recipe)
{
	std::vector<char> expected_data;
	expected_data.reserve(c_letters_in_alphabet);
	std::memcpy(expected_data.data(), c_alphabet, c_letters_in_alphabet);

	item_definition alphabet_item{create_definition_from_vector_using_capacity(expected_data)};
	// auto expected_hash = alphabet_item.m_hashes[0];

	auto data1 = std::make_shared<std::vector<char>>();

	for (size_t len1 = 1; len1 < (c_letters_in_alphabet - 2); len1++)
	{
		data1->reserve(len1);
		std::memcpy(data1->data(), c_alphabet, len1);
		item_definition part1{create_definition_from_vector_using_capacity(*data1)};

		for (size_t len2 = 1; len2 < (c_letters_in_alphabet - (len1 + 1)); len2++)
		{
			auto data2     = std::make_shared<std::vector<char>>();
			size_t offset2 = len1;
			data2->reserve(len2);
			std::memcpy(data2->data(), c_alphabet + offset2, len2);
			item_definition part2{create_definition_from_vector_using_capacity(*data2)};

			auto data3     = std::make_shared<std::vector<char>>();
			size_t len3    = c_letters_in_alphabet - (len1 + len2);
			size_t offset3 = offset2 + len2;
			data3->reserve(len3);
			std::memcpy(data3->data(), c_alphabet + offset3, len3);
			item_definition part3{create_definition_from_vector_using_capacity(*data3)};

			archive_diff::diffs::recipes::basic::chain_recipe::recipe_template recipe_template{};

			std::vector<uint64_t> number_ingredients;
			std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
			item_ingredients.push_back(part1);
			item_ingredients.push_back(part2);
			item_ingredients.push_back(part3);

			auto recipe = recipe_template.create_recipe(alphabet_item, number_ingredients, item_ingredients);

			// verify there are 3 item ingredients
			auto item_ingredients_found = recipe->get_item_ingredients();
			ASSERT_EQ(3, item_ingredients_found.size());

			using device = archive_diff::io::buffer::io_device;
			std::shared_ptr<archive_diff::io::reader_factory> part1_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data1, device::size_kind::vector_capacity);
			std::shared_ptr<archive_diff::io::reader_factory> part2_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data2, device::size_kind::vector_capacity);
			std::shared_ptr<archive_diff::io::reader_factory> part3_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data3, device::size_kind::vector_capacity);

			auto prep_part1 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part1, archive_diff::diffs::core::prepared_item::reader_kind{part1_factory});
			auto prep_part2 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part2, archive_diff::diffs::core::prepared_item::reader_kind{part2_factory});
			auto prep_part3 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part3, archive_diff::diffs::core::prepared_item::reader_kind{part3_factory});

			auto kitchen = archive_diff::diffs::core::kitchen::create();
			kitchen->store_item(prep_part1);
			kitchen->store_item(prep_part2);
			kitchen->store_item(prep_part3);
			kitchen->add_recipe(recipe);

			kitchen->request_item(alphabet_item);
			kitchen->process_requested_items();

			std::vector<std::shared_ptr<archive_diff::diffs::core::prepared_item>> prepared_ingredients{
				prep_part1, prep_part2, prep_part3};
			auto prepared_item = recipe->prepare(kitchen.get(), prepared_ingredients);

			ASSERT_EQ(true, prepared_item->can_make_reader());

			auto reader = prepared_item->make_reader();
			ASSERT_EQ(c_letters_in_alphabet, reader.size());
			std::vector<char> actual_data;
			actual_data.reserve(static_cast<size_t>(reader.size()));
			reader.read(0, std::span<char>{actual_data.data(), actual_data.capacity()});
			ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), c_letters_in_alphabet));

			auto sequential_reader = prepared_item->make_sequential_reader();
			ASSERT_EQ(c_letters_in_alphabet, sequential_reader->size());
			std::memset(actual_data.data(), 0, static_cast<size_t>(sequential_reader->size()));
			sequential_reader->read(std::span<char>{actual_data.data(), actual_data.capacity()});
			ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), c_letters_in_alphabet));
		}
	}
}

TEST(chain_recipe, fetch_from_kitchen)
{
	std::vector<char> expected_data;
	expected_data.reserve(c_letters_in_alphabet);
	std::memcpy(expected_data.data(), c_alphabet, c_letters_in_alphabet);

	item_definition alphabet_item{create_definition_from_vector_using_capacity(expected_data)};
	//	auto expected_hash = alphabet_item.m_hashes[0];

	auto data1 = std::make_shared<std::vector<char>>();

	for (size_t len1 = 1; len1 < (c_letters_in_alphabet - 2); len1++)
	{
		data1->reserve(len1);
		std::memcpy(data1->data(), c_alphabet, len1);
		item_definition part1{create_definition_from_vector_using_capacity(*data1)};

		for (size_t len2 = 1; len2 < (c_letters_in_alphabet - (len1 + 1)); len2++)
		{
			auto data2     = std::make_shared<std::vector<char>>();
			size_t offset2 = len1;
			data2->reserve(len2);
			std::memcpy(data2->data(), c_alphabet + offset2, len2);
			item_definition part2{create_definition_from_vector_using_capacity(*data2)};

			auto data3     = std::make_shared<std::vector<char>>();
			size_t len3    = c_letters_in_alphabet - (len1 + len2);
			size_t offset3 = offset2 + len2;
			data3->reserve(len3);
			std::memcpy(data3->data(), c_alphabet + offset3, len3);
			item_definition part3{create_definition_from_vector_using_capacity(*data3)};

			archive_diff::diffs::recipes::basic::chain_recipe::recipe_template recipe_template{};

			std::vector<uint64_t> number_ingredients;
			std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
			item_ingredients.push_back(part1);
			item_ingredients.push_back(part2);
			item_ingredients.push_back(part3);

			auto recipe = recipe_template.create_recipe(alphabet_item, number_ingredients, item_ingredients);

			// verify there are 3 item ingredients
			auto item_ingredients_found = recipe->get_item_ingredients();
			ASSERT_EQ(3, item_ingredients_found.size());

			// prepare the recipe
			auto kitchen = archive_diff::diffs::core::kitchen::create();
			kitchen->add_recipe(recipe);

			ASSERT_EQ(false, kitchen->can_fetch_item(alphabet_item));

			using device = archive_diff::io::buffer::io_device;
			std::shared_ptr<archive_diff::io::reader_factory> part1_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data1, device::size_kind::vector_capacity);
			std::shared_ptr<archive_diff::io::reader_factory> part2_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data2, device::size_kind::vector_capacity);
			std::shared_ptr<archive_diff::io::reader_factory> part3_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data3, device::size_kind::vector_capacity);

			auto prep_part1 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part1, archive_diff::diffs::core::prepared_item::reader_kind{part1_factory});
			auto prep_part2 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part2, archive_diff::diffs::core::prepared_item::reader_kind{part2_factory});
			auto prep_part3 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part3, archive_diff::diffs::core::prepared_item::reader_kind{part3_factory});

			kitchen->store_item(prep_part1);
			kitchen->store_item(prep_part2);
			kitchen->store_item(prep_part3);

			ASSERT_FALSE(kitchen->can_fetch_item(alphabet_item));

			kitchen->request_item(alphabet_item);
			kitchen->process_requested_items();

			ASSERT_TRUE(kitchen->can_fetch_item(alphabet_item));

			auto prepared_item = kitchen->fetch_item(alphabet_item);

			ASSERT_TRUE(prepared_item->can_make_reader());

			auto reader = prepared_item->make_reader();
			ASSERT_EQ(c_letters_in_alphabet, reader.size());
			std::vector<char> actual_data;
			actual_data.reserve(static_cast<size_t>(reader.size()));
			reader.read(0, std::span<char>{actual_data.data(), actual_data.capacity()});
			ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), c_letters_in_alphabet));

			auto sequential_reader = prepared_item->make_sequential_reader();
			ASSERT_EQ(c_letters_in_alphabet, sequential_reader->size());
			std::memset(actual_data.data(), 0, static_cast<size_t>(sequential_reader->size()));
			sequential_reader->read(std::span<char>{actual_data.data(), actual_data.capacity()});
			ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), c_letters_in_alphabet));
		}
	}
}