/**
 * @file test_slice_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <hashing/hasher.h>

#include <io/buffer/reader_factory.h>
#include <io/sequential/basic_reader_wrapper.h>

#include <diffs/recipes/basic/chain_recipe.h>
#include <diffs/recipes/basic/slice_recipe.h>

#include <diffs/core/kitchen.h>

#include "common.h"

TEST(slice, fetch_slice_of_reader)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> alphabet_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(alphabet_vector, device::size_kind::vector_capacity);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::reader_kind{alphabet_factory});

	for (size_t offset = 0; offset < c_letters_in_alphabet - 1; offset += 7)
	{
		for (size_t len_big_slice = 1; (offset + len_big_slice) < c_letters_in_alphabet; len_big_slice += 5)
		{
			auto data_big_slice = std::string_view{alphabet_data.data() + offset, len_big_slice};
			item_definition big_slice_item{create_definition_from_data(data_big_slice)};
			auto vector_big_slice = std::make_shared<std::vector<char>>();
			vector_big_slice->reserve(len_big_slice);
			std::memcpy(vector_big_slice->data(), data_big_slice.data(), data_big_slice.size());

			std::shared_ptr<archive_diff::io::reader_factory> big_slice_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(
					vector_big_slice, device::size_kind::vector_capacity);

			archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

			std::vector<uint64_t> big_slice_number_ingredients;
			big_slice_number_ingredients.push_back(offset);
			std::vector<archive_diff::diffs::core::item_definition> big_slice_item_ingredients;
			big_slice_item_ingredients.push_back(alphabet_item);
			auto big_slice_recipe = slice_recipe_template.create_recipe(
				big_slice_item, big_slice_number_ingredients, big_slice_item_ingredients);

			auto kitchen = archive_diff::diffs::core::kitchen::create();
			ASSERT_FALSE(kitchen->can_fetch_item(big_slice_item));

			kitchen->add_recipe(big_slice_recipe);
			ASSERT_FALSE(kitchen->can_fetch_item(big_slice_item));

			kitchen->store_item(prep_alphabet);

			kitchen->request_item(big_slice_item);
			ASSERT_TRUE(kitchen->process_requested_items());
			ASSERT_TRUE(kitchen->can_fetch_item(big_slice_item));

			auto prep_big_slice = kitchen->fetch_item(big_slice_item);
			ASSERT_TRUE(prep_big_slice->can_make_reader());

			auto reader = prep_big_slice->make_reader();
			ASSERT_EQ(len_big_slice, reader.size());

			std::vector<char> result;
			reader.read_all(result);

			ASSERT_EQ(0, std::memcmp(result.data(), data_big_slice.data(), len_big_slice));
		}
	}
}

TEST(slice, fetch_slice_of_slice)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> alphabet_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(alphabet_vector, device::size_kind::vector_capacity);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::reader_kind{alphabet_factory});

	for (size_t offset = 0; offset < c_letters_in_alphabet - 1; offset += 7)
	{
		for (size_t len_big_slice = 1; (offset + len_big_slice) < c_letters_in_alphabet; len_big_slice += 5)
		{
			auto data_big_slice = std::string_view{alphabet_data.data() + offset, len_big_slice};
			item_definition big_slice_item{create_definition_from_data(data_big_slice)};
			auto vector_big_slice = std::make_shared<std::vector<char>>();
			vector_big_slice->reserve(len_big_slice);
			std::memcpy(vector_big_slice->data(), data_big_slice.data(), data_big_slice.size());

			std::shared_ptr<archive_diff::io::reader_factory> big_slice_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(
					vector_big_slice, device::size_kind::vector_capacity);

			archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

			std::vector<uint64_t> big_slice_number_ingredients;
			big_slice_number_ingredients.push_back(offset);
			std::vector<archive_diff::diffs::core::item_definition> big_slice_item_ingredients;
			big_slice_item_ingredients.push_back(alphabet_item);
			auto big_slice_recipe = slice_recipe_template.create_recipe(
				big_slice_item, big_slice_number_ingredients, big_slice_item_ingredients);

			for (size_t offset_within = 0; offset_within < (len_big_slice - 1); offset_within += 3)
			{
				for (size_t len_small_slice = 1; (len_small_slice + offset_within) <= len_big_slice; len_small_slice++)
				{
					auto data_small_slice = std::string_view{data_big_slice.data() + offset_within, len_small_slice};
					item_definition small_slice_item{create_definition_from_data(data_small_slice)};
					auto vector_small_slice = std::make_shared<std::vector<char>>();
					vector_small_slice->reserve(len_small_slice);
					std::memcpy(vector_small_slice->data(), data_small_slice.data(), data_small_slice.size());

					auto kitchen = archive_diff::diffs::core::kitchen::create();
					ASSERT_FALSE(kitchen->can_fetch_item(small_slice_item));

					std::vector<uint64_t> small_slice_number_ingredients;
					small_slice_number_ingredients.push_back(offset_within);
					std::vector<archive_diff::diffs::core::item_definition> small_slice_item_ingredients;
					small_slice_item_ingredients.push_back(big_slice_item);

					if (small_slice_item.size() == big_slice_item.size())
					{
						bool caught_exception{false};
						try
						{
							auto self_referential_recipe = slice_recipe_template.create_recipe(
								small_slice_item, small_slice_number_ingredients, small_slice_item_ingredients);
						}
						catch (archive_diff::errors::user_exception &e)
						{
							if (e.get_error() == archive_diff::errors::error_code::recipe_self_referential)
							{
								caught_exception = true;
							}
						}
						ASSERT_TRUE(caught_exception);
						continue;
					}

					auto small_slice_recipe = slice_recipe_template.create_recipe(
						small_slice_item, small_slice_number_ingredients, small_slice_item_ingredients);

					kitchen->add_recipe(small_slice_recipe);
					ASSERT_FALSE(kitchen->can_fetch_item(small_slice_item));

					kitchen->add_recipe(big_slice_recipe);
					ASSERT_FALSE(kitchen->can_fetch_item(small_slice_item));

					kitchen->store_item(prep_alphabet);

					kitchen->request_item(small_slice_item);
					ASSERT_TRUE(kitchen->process_requested_items());
					ASSERT_TRUE(kitchen->can_fetch_item(small_slice_item));

					auto prep_small_slice = kitchen->fetch_item(small_slice_item);
					ASSERT_TRUE(prep_small_slice->can_make_reader());

					auto reader = prep_small_slice->make_reader();
					ASSERT_EQ(len_small_slice, reader.size());

					std::vector<char> result;
					reader.read_all(result);

					ASSERT_EQ(0, std::memcmp(result.data(), data_small_slice.data(), len_small_slice));
				}
			}
		}
	}
}

TEST(slice, fetch_slice_of_chain)
{
	std::vector<char> alphabet_data;
	alphabet_data.reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_data.data(), c_alphabet, c_letters_in_alphabet);

	using device = archive_diff::io::buffer::io_device;

	item_definition alphabet_item{create_definition_from_vector_using_capacity(alphabet_data)};

	auto data1 = std::make_shared<std::vector<char>>();

	archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

	for (size_t len1 = 1; len1 < (c_letters_in_alphabet - 2); len1++)
	{
		data1->reserve(len1);
		std::memcpy(data1->data(), c_alphabet, len1);
		item_definition part1{create_definition_from_vector_using_capacity(*data1)};

		std::shared_ptr<archive_diff::io::reader_factory> part1_factory =
			std::make_shared<archive_diff::io::buffer::reader_factory>(data1, device::size_kind::vector_capacity);

		auto prep_part1 = std::make_shared<archive_diff::diffs::core::prepared_item>(
			part1, archive_diff::diffs::core::prepared_item::reader_kind{part1_factory});

		// step by 7 to help with run-time
		for (size_t len2 = 1; len2 < (c_letters_in_alphabet - (len1 + 1)); len2 += 7)
		{
			auto data2     = std::make_shared<std::vector<char>>();
			size_t offset2 = len1;
			data2->reserve(len2);
			std::memcpy(data2->data(), c_alphabet + offset2, len2);
			item_definition part2{create_definition_from_vector_using_capacity(*data2)};

			std::shared_ptr<archive_diff::io::reader_factory> part2_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data2, device::size_kind::vector_capacity);
			auto prep_part2 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part2, archive_diff::diffs::core::prepared_item::reader_kind{part2_factory});

			auto data3     = std::make_shared<std::vector<char>>();
			size_t len3    = c_letters_in_alphabet - (len1 + len2);
			size_t offset3 = offset2 + len2;
			data3->reserve(len3);
			std::memcpy(data3->data(), c_alphabet + offset3, len3);
			item_definition part3{create_definition_from_vector_using_capacity(*data3)};

			std::shared_ptr<archive_diff::io::reader_factory> part3_factory =
				std::make_shared<archive_diff::io::buffer::reader_factory>(data3, device::size_kind::vector_capacity);
			auto prep_part3 = std::make_shared<archive_diff::diffs::core::prepared_item>(
				part3, archive_diff::diffs::core::prepared_item::reader_kind{part3_factory});

			archive_diff::diffs::recipes::basic::chain_recipe::recipe_template recipe_template{};

			std::vector<uint64_t> number_ingredients;
			std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
			item_ingredients.push_back(part1);
			item_ingredients.push_back(part2);
			item_ingredients.push_back(part3);

			auto chain_recipe = recipe_template.create_recipe(alphabet_item, number_ingredients, item_ingredients);

			// verify there are 3 item ingredients
			auto item_ingredients_found = chain_recipe->get_item_ingredients();
			ASSERT_EQ(3, item_ingredients_found.size());

			for (uint64_t slice_offset = 0; slice_offset < (c_letters_in_alphabet - 1); slice_offset++)
			{
				// step by 5 to help with run-time
				for (uint64_t slice_length = 1; (slice_offset + slice_length) < c_letters_in_alphabet;
				     slice_length += 5)
				{
					std::vector<char> expected_data{};
					expected_data.reserve(slice_length);
					std::memcpy(expected_data.data(), c_alphabet + slice_offset, slice_length);

					item_definition slice_item{create_definition_from_vector_using_capacity(expected_data)};
					// auto expected_hash = slice_item.m_hashes[0];

					auto kitchen = archive_diff::diffs::core::kitchen::create();
					ASSERT_EQ(false, kitchen->can_fetch_item(slice_item));

					std::vector<uint64_t> slice_number_ingredients{{slice_offset}};
					std::vector<archive_diff::diffs::core::item_definition> slice_item_ingredients{{alphabet_item}};

					auto slice_recipe = slice_recipe_template.create_recipe(
						slice_item, slice_number_ingredients, slice_item_ingredients);
					kitchen->add_recipe(slice_recipe);
					ASSERT_EQ(false, kitchen->can_fetch_item(slice_item));

					kitchen->add_recipe(chain_recipe);

					ASSERT_EQ(false, kitchen->can_fetch_item(slice_item));

					kitchen->store_item(prep_part1);
					kitchen->store_item(prep_part2);
					kitchen->store_item(prep_part3);

					kitchen->request_item(slice_item);
					ASSERT_TRUE(kitchen->process_requested_items());

					ASSERT_TRUE(kitchen->can_fetch_item(slice_item));

					auto prepared_item = kitchen->fetch_item(slice_item);

					ASSERT_TRUE(prepared_item->can_make_reader());

					auto reader = prepared_item->make_reader();
					ASSERT_EQ(slice_length, reader.size());

					std::vector<char> actual_data;
					actual_data.reserve(reader.size());
					reader.read(0, std::span<char>{actual_data.data(), actual_data.capacity()});

					ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), slice_length));

					auto sequential_reader = prepared_item->make_sequential_reader();
					ASSERT_EQ(slice_length, sequential_reader->size());
					std::memset(actual_data.data(), 0, sequential_reader->size());
					sequential_reader->read(std::span<char>{actual_data.data(), actual_data.capacity()});

					ASSERT_EQ(0, std::memcmp(actual_data.data(), expected_data.data(), slice_length));
				}
			}
		}
	}
}

//
// most deep level is a sequential reader that contains: "abcdefghijklmnopqrstuvwxyz"
// next we have recipes yielding each of the letters singly
// next we spell out "the quick brown fox jumps over the lazy dog" using a chain of the the single parts (skipping
// spaces) lastly we define slices for "the" (one for each spot), "fox", "dog" and then "lazy" twice We use a custom
// reader for the alphabet here to synthetically make the deepest level slow
//
TEST(slice_recipe, tiered_slicing_and_chaining)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device = archive_diff::io::buffer::io_device;
	std::shared_ptr<archive_diff::io::reader_factory> alphabet_factory =
		std::make_shared<archive_diff::io::buffer::reader_factory>(alphabet_vector, device::size_kind::vector_capacity);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::reader_kind{alphabet_factory});

	// Slice each letter individually
	std::vector<item_definition> slice_items;

	auto kitchen = archive_diff::diffs::core::kitchen::create();
	archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

	kitchen->store_item(prep_alphabet);

	for (size_t i = 0; i < c_letters_in_alphabet; i++)
	{
		slice_items.push_back(create_definition_from_data(std::string_view{alphabet_data.data() + i, 1}));
		ASSERT_FALSE(kitchen->can_fetch_item(slice_items[i]));

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(i);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		item_ingredients.push_back(alphabet_item);

		auto slice_recipe = slice_recipe_template.create_recipe(slice_items[i], number_ingredients, item_ingredients);
		kitchen->add_recipe(slice_recipe);

		kitchen->request_item(slice_items[i]);
		ASSERT_TRUE(kitchen->process_requested_items());

		ASSERT_TRUE(kitchen->can_fetch_item(slice_items[i]));

		// printf("%c: %s\n", (char) ('a' + i), slice_items[i].to_string().c_str());
	}

	// create a recipe for our string
	const std::string test_string = "thequickbrownfoxjumpsoverthelazydog";

	std::vector<item_definition> test_string_item_ingredients;

	auto test_string_item = create_definition_from_data(std::string_view{test_string.c_str(), test_string.size()});

	for (auto letter : test_string)
	{
		auto letter_index = letter - 'a';
		auto letter_item  = slice_items[letter_index];

		test_string_item_ingredients.push_back(letter_item);
	}

	std::vector<uint64_t> test_string_number_ingredients;

	archive_diff::diffs::recipes::basic::chain_recipe::recipe_template chain_recipe_template{};
	auto test_string_recipe = chain_recipe_template.create_recipe(
		test_string_item, test_string_number_ingredients, test_string_item_ingredients);

	kitchen->add_recipe(test_string_recipe);
	kitchen->request_item(test_string_item);
	ASSERT_TRUE(kitchen->process_requested_items());

	ASSERT_TRUE(kitchen->can_fetch_item(test_string_item));

	// thequickbrownfoxjumpsoverthelazydog
	// 01234567890123456789012345678901234
	// lastly we define slices for "the", "fox", "dog" and then "lazy"
	const std::string parts_of_test_string[] = {"the", "fox", "dog", "lazy"};

	std::vector<item_definition> part_items;

	for (auto &part_string : parts_of_test_string)
	{
		auto pos = test_string.find(part_string);

		ASSERT_NE(pos, std::string::npos);

		// printf("Found %s at pos %d\n", part_string.c_str(), (int)pos);

		part_items.push_back(create_definition_from_data(std::string_view{part_string.data(), part_string.size()}));

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(pos);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		item_ingredients.push_back(test_string_item);

		auto part_recipe = slice_recipe_template.create_recipe(part_items.back(), number_ingredients, item_ingredients);

		kitchen->add_recipe(part_recipe);
		kitchen->request_item(part_items.back());
		// printf("%s: %s\n", part_string.c_str(), part_items.back().to_string().c_str());
	}

	ASSERT_TRUE(kitchen->process_requested_items());

	for (size_t i = 0; i < parts_of_test_string->size(); i++)
	{
		auto &part_string = parts_of_test_string[i];
		auto &part_item   = part_items[i];

		ASSERT_TRUE(kitchen->can_fetch_item(part_item));

		// printf("Going to fetch: %s\n", part_item.to_string().c_str());
		auto prep_part = kitchen->fetch_item(part_item);

		ASSERT_TRUE(prep_part->can_make_reader());

		auto reader = prep_part->make_reader();

		std::vector<char> result;
		reader.read_all(result);

		ASSERT_EQ(reader.size(), part_string.size());
		ASSERT_EQ(0, std::memcmp(result.data(), part_string.data(), part_string.size()));
	}
}