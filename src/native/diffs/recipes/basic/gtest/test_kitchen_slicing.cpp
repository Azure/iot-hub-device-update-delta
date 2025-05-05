/**
 * @file test_kitchen_slicing.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <chrono>
#include <thread>

#include <errors/user_exception.h>

#include <test_utility/gtest_includes.h>
#include <test_utility/buffer_helpers.h>

#include <hashing/hasher.h>

#include <io/buffer/reader_factory.h>
#include <io/sequential/basic_reader_wrapper.h>

#include <diffs/recipes/basic/chain_recipe.h>
#include <diffs/recipes/basic/slice_recipe.h>

#include <diffs/core/kitchen.h>

#include "common.h"

#if 0

class sequential_reader_factory_of_reader : public archive_diff::io::sequential::reader_factory
{
	public:
	sequential_reader_factory_of_reader(archive_diff::io::reader &reader) : m_reader(reader) {}

	virtual std::unique_ptr<archive_diff::io::sequential::reader> make_sequential_reader() override
	{
		return std::make_unique<archive_diff::io::sequential::basic_reader_wrapper>(m_reader);
	}

	private:
	archive_diff::io::reader m_reader;
};

void test_maybe_overlapping_slices(std::vector<std::tuple<uint64_t, uint64_t, bool>> &test_data)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device         = archive_diff::io::buffer::io_device;
	auto alphabet_reader = device::make_reader(alphabet_vector, device::size_kind::vector_capacity);
	std::shared_ptr<archive_diff::io::sequential::reader_factory> alphabet_factory =
		std::make_shared<sequential_reader_factory_of_reader>(alphabet_reader);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::sequential_reader_kind{alphabet_factory});

	std::vector<archive_diff::diffs::core::item_definition> slice_items;
	std::vector<std::string_view> slice_data;
	std::vector<std::shared_ptr<archive_diff::diffs::core::recipe>> slice_recipes;
	std::vector<std::shared_ptr<archive_diff::diffs::core::prepared_item>> slice_prepared_items;

	archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

	auto kitchen = archive_diff::diffs::core::kitchen::create();

	ASSERT_FALSE(kitchen->can_fetch_item(alphabet_item));

	// printf("test_data:\n");
	for (auto &test : test_data)
	{
		auto offset = std::get<0>(test);
		auto length = static_cast<size_t>(std::get<1>(test));
		// auto expected = std::get<2>(test);
		// printf("\toffset: %d, length: %d, expected: %s\n", (int)offset, (int)length, expected ? "success" :
		// "failure");

		slice_data.push_back(std::string_view{alphabet_data.data() + offset, length});
		slice_items.push_back(create_definition_from_data(slice_data.back()));

		ASSERT_FALSE(kitchen->can_fetch_item(slice_items.back()));

		std::vector<uint64_t> number_ingredients;
		std::vector<item_definition> item_ingredients;
		number_ingredients.push_back(offset);
		item_ingredients.push_back(alphabet_item);
		auto recipe = slice_recipe_template.create_recipe(slice_items.back(), number_ingredients, item_ingredients);

		slice_recipes.push_back(recipe);
	}

	kitchen->store_item(prep_alphabet);

	// Try to get everything we expect to succeed

	for (size_t i = 0; i < slice_items.size(); i++)
	{
		auto expected_success = std::get<2>(test_data[i]);
		if (!expected_success)
		{
			continue;
		}
		kitchen->add_recipe(slice_recipes[i]);
		kitchen->request_item(slice_items[i]);
	}

	kitchen->process_requested_items();

	kitchen->resume_slicing();

	// now that we started slicing, verify that everything we expected
	// to succeed does fetch a reader with the correct results
	for (size_t i = 0; i < slice_items.size(); i++)
	{
		auto expected_success = std::get<2>(test_data[i]);
		if (!expected_success)
		{
			continue;
		}

		auto prep_item = kitchen->fetch_slice(slice_items[i]);

		auto reader = prep_item->make_reader();

		ASSERT_EQ(reader.size(), slice_data[i].size());
		std::vector<char> result;
		reader.read_all(result);
		ASSERT_EQ(result.size(), slice_data[i].size());
		ASSERT_EQ(0, std::memcmp(result.data(), slice_data[i].data(), result.size()));
	}

	// ok, now try to process the requests and verify we fail as long as there are any
	// expected overlaps

	kitchen = archive_diff::diffs::core::kitchen::create();
	kitchen->store_item(prep_alphabet);
	bool found_expected_failure{false};
	for (size_t i = 0; i < slice_items.size(); i++)
	{
		auto expected_success = std::get<2>(test_data[i]);
		if (!expected_success)
		{
			found_expected_failure = true;
		}
		kitchen->add_recipe(slice_recipes[i]);
		kitchen->request_item(slice_items[i]);
	}

	if (!found_expected_failure)
	{
		return;
	}

	bool caught_exception{false};
	try
	{
		kitchen->process_requested_items();
	}
	catch (archive_diff::errors::user_exception &e)
	{
		if (e.get_error() == archive_diff::errors::error_code::diff_slicing_request_slice_overlap)
		{
			caught_exception = true;
		}
	}
	ASSERT_TRUE(caught_exception);
}

TEST(kitchen_slicing, overlapping_slices)
{
	std::vector<std::tuple<uint64_t, uint64_t, bool>> test_data[]{
		// complete alphabet read, but no overlap
		{{0, 5, true}, {5, 5, true}, {10, 5, true}, {15, 5, true}, {20, 5, true}, {25, 1, true}},
		// some gaps but no overlap
		{{0, 10, true}, {12, 5, true}, {19, 3, true}, {25, 1, true}},
		// immediate overlap on second slice
		{{0, 10, true}, {5, 5, false}},
		// immediate overlap on second slice, from other side
		{{5, 5, true}, {0, 10, false}},
		// immediate, slight overlap on second slice
		{{0, 5, true}, {4, 5, false}},
		// immediate, slight overlap on second slice, from other side
		{{4, 5, true}, {0, 5, false}},
		// immediate, slight overlap on third slice
		{{0, 1, true}, {5, 5, true}, {9, 5, false}},
		// immediate, slight overlap on third slice, from other side
		{{0, 1, true}, {9, 5, true}, {5, 5, false}},
		// immediate, slight overlap on second slice, with non-overlap after
		{{0, 5, true}, {4, 5, false}, {9, 5, true}},
		// immediate, slight overlap on second slice, from other side, with non-overlap after
		{{4, 5, true}, {0, 5, false}, {9, 5, true}},
		// immediate, slight overlap on third slice, with non-overlap after
		{{0, 1, true}, {5, 5, true}, {9, 5, false}, {14, 5, true}},
		// immediate, slight overlap on third slice, from other side, with non-overlap after
		{{0, 1, true}, {9, 5, true}, {5, 5, false}, {14, 5, true}},
	};

	for (auto &scenario : test_data)
	{
		test_maybe_overlapping_slices(scenario);
	}
}

using item_definition = archive_diff::diffs::core::item_definition;
using recipe          = archive_diff::diffs::core::recipe;
using kitchen         = archive_diff::diffs::core::kitchen;
using prepared_item   = archive_diff::diffs::core::prepared_item;

class ceasar_cipher_sequential_reader : public archive_diff::io::sequential::reader
{
	public:
	ceasar_cipher_sequential_reader(
		std::unique_ptr<archive_diff::io::sequential::reader> &&reader,
		uint8_t rotation,
		std::chrono::duration<uint64_t, std::milli> sleep_on_call) :
		m_reader(std::move(reader)), m_rotation(rotation), m_sleep_on_call(sleep_on_call)
	{}

	virtual void skip(uint64_t to_skip) override { m_reader->skip(to_skip); }
	virtual size_t read_some(std::span<char> buffer) override
	{
		std::this_thread::sleep_for(m_sleep_on_call);
		auto actual_read = m_reader->read_some(buffer);

		for (size_t i = 0; i < actual_read; i++)
		{
			buffer[i] = rotate(buffer[i]);
		}
		return actual_read;
	}
	virtual uint64_t tellg() const override { return m_reader->tellg(); }
	virtual uint64_t size() const { return m_reader->size(); }

	private:
	uint8_t rotate(uint16_t c) { return static_cast<uint8_t>((c + m_rotation % 256)); }

	std::unique_ptr<archive_diff::io::sequential::reader> m_reader;
	uint8_t m_rotation;
	std::chrono::duration<uint64_t, std::milli> m_sleep_on_call;
};

class ceasar_cipher_sequential_reader_factory : public archive_diff::io::sequential::reader_factory
{
	public:
	ceasar_cipher_sequential_reader_factory(
		std::shared_ptr<archive_diff::diffs::core::prepared_item> &clear_text_prepared_item,
		uint8_t rotation,
		std::chrono::duration<uint64_t, std::milli> sleep_on_call) :
		m_clear_text_prepared_item(clear_text_prepared_item), m_rotation(rotation), m_sleep_on_call(sleep_on_call)
	{}

	virtual std::unique_ptr<archive_diff::io::sequential::reader> make_sequential_reader()
	{
		auto clear_text_reader = m_clear_text_prepared_item->make_sequential_reader();

		return std::make_unique<ceasar_cipher_sequential_reader>(
			std::move(clear_text_reader), m_rotation, m_sleep_on_call);
	}

	private:
	std::shared_ptr<archive_diff::diffs::core::prepared_item> m_clear_text_prepared_item;
	uint8_t m_rotation;
	std::chrono::duration<uint64_t, std::milli> m_sleep_on_call;
};

class caeasar_cipher_recipe : public recipe
{
	public:
	caeasar_cipher_recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients) :
		recipe(result_item_definition, number_ingredients, item_ingredients)
	{
		if (number_ingredients.size() != 2)
		{
			throw std::exception();
		}

		if (item_ingredients.size() != 1)
		{
			throw std::exception();
		}

		m_rotation      = number_ingredients[0] & 0xFF;
		m_sleep_on_call = std::chrono::duration<uint64_t, std::milli>(number_ingredients[1]);

		m_clear_text_item = item_ingredients[0];
	}

	virtual std::string get_recipe_name() const override { return "slice"; }

	protected:
	archive_diff::diffs::core::recipe::prepare_result prepare(
		[[maybe_unused]] kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
	{
		auto clear_text_item = items[0];

		std::shared_ptr<archive_diff::io::sequential::reader_factory> factory;
		factory =
			std::make_shared<ceasar_cipher_sequential_reader_factory>(clear_text_item, m_rotation, m_sleep_on_call);

		return std::make_shared<prepared_item>(
			m_result_item_definition, archive_diff::diffs::core::prepared_item::sequential_reader_kind{factory});
	}

	private:
	uint8_t m_rotation;
	std::chrono::duration<uint64_t, std::milli> m_sleep_on_call;
	item_definition m_clear_text_item;
};

using caeasar_cipher_recipe_recipe_template = archive_diff::diffs::core::recipe_template_impl<caeasar_cipher_recipe>;

TEST(kitchen_slicing, slices_of_cipher_of_chained_slices)
{
	// Take the alphabet and then break it up into a bunch of slices...
	// Then we take a slice that into several pieces and create a new thing
	// We then apply a caesar cipher to the chain
	// then we slice the cipher thing into several pieces
	// the alphabet is populated with a sequential reader, so slicing the chain should require kitchen slicing
	// we implement the caeser cipher so that it outputs a sequential reader, so the slicing there should require
	// kitchen slicing

	// Take the alphabet and then break it up into a bunch of slices...

	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device         = archive_diff::io::buffer::io_device;
	auto alphabet_reader = device::make_reader(alphabet_vector, device::size_kind::vector_capacity);
	std::shared_ptr<archive_diff::io::sequential::reader_factory> alphabet_factory =
		std::make_shared<sequential_reader_factory_of_reader>(alphabet_reader);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::sequential_reader_kind{alphabet_factory});

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
		ASSERT_FALSE(kitchen->can_fetch_item(slice_items[i]));
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
	auto test_string_prep = kitchen->fetch_item(test_string_item);

	// thequickbrownfoxjumpsoverthelazydog
	// 01234567890123456789012345678901234
	// we define slices for "the", "fox", "dog" and then "lazy"
	const std::string parts_of_test_string[] = {"the", "fox", "dog", "lazy"};

	std::vector<item_definition> part_items;

	std::string all_parts_together;

	for (auto &part_string : parts_of_test_string)
	{
		auto pos = test_string.find(part_string);

		ASSERT_NE(pos, std::string::npos);

		// printf("Found %s at pos %d\n", part_string.c_str(), (int)pos);

		auto part_view = std::string_view{part_string.data(), part_string.size()};
		part_items.push_back(create_definition_from_data(part_view));

		std::vector<uint64_t> number_ingredients;
		number_ingredients.push_back(pos);
		std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
		item_ingredients.push_back(test_string_item);

		auto part_recipe = slice_recipe_template.create_recipe(part_items.back(), number_ingredients, item_ingredients);

		kitchen->add_recipe(part_recipe);
		// printf("%s: %s\n", part_string.c_str(), part_items.back().to_string().c_str());

		all_parts_together += part_view;
	}

	auto all_parts_view = std::string_view{all_parts_together.data(), all_parts_together.size()};
	auto all_parts_item = create_definition_from_data(all_parts_view);

	std::vector<uint64_t> number_ingredients;
	auto all_parts_chain_recipe = chain_recipe_template.create_recipe(all_parts_item, number_ingredients, part_items);

	ASSERT_FALSE(kitchen->can_fetch_item(all_parts_item));

	kitchen->add_recipe(all_parts_chain_recipe);
	kitchen->request_item(all_parts_item);
	ASSERT_TRUE(kitchen->process_requested_items());
	ASSERT_TRUE(kitchen->can_fetch_item(all_parts_item));

	std::vector<uint64_t> caesar_cipher_number_ingredients{224, 0};
	std::vector<item_definition> caesar_cipher_item_ingredients{all_parts_item};

	std::string cipher_result_string;
	for (auto letter : all_parts_together)
	{
		cipher_result_string.push_back(letter - 32);
	}
	auto cipher_data = std::string_view{cipher_result_string.data(), cipher_result_string.size()};

	auto cipher_result_item = create_definition_from_data(cipher_data);

	caeasar_cipher_recipe_recipe_template caesar_recipe_template;

	auto cipher_recipe = caesar_recipe_template.create_recipe(
		cipher_result_item, caesar_cipher_number_ingredients, caesar_cipher_item_ingredients);

	ASSERT_FALSE(kitchen->can_fetch_item(cipher_result_item));

	kitchen->add_recipe(cipher_recipe);

	ASSERT_FALSE(kitchen->can_fetch_item(cipher_result_item));

	std::string_view slice_of_cipher_data{cipher_data.data() + 5, 5};

	auto slice_of_cipher_item = create_definition_from_data(slice_of_cipher_data);

	std::vector<uint64_t> slice_of_cipher_number_ingredients{5};
	std::vector<item_definition> slice_of_cipher_item_ingredients{cipher_result_item};

	auto slice_of_cipher_recipe = slice_recipe_template.create_recipe(
		slice_of_cipher_item, slice_of_cipher_number_ingredients, slice_of_cipher_item_ingredients);

	ASSERT_FALSE(kitchen->can_fetch_item(slice_of_cipher_item));

	kitchen->add_recipe(slice_of_cipher_recipe);

	ASSERT_FALSE(kitchen->can_fetch_item(slice_of_cipher_item));

	kitchen->request_item(slice_of_cipher_item);
	ASSERT_TRUE(kitchen->process_requested_items());

	ASSERT_TRUE(kitchen->can_fetch_item(slice_of_cipher_item));

	auto prep_cipher_slice = kitchen->fetch_item(slice_of_cipher_item);

	// this needs slicing, we should expect it to fail
	bool caught_exception{false};
	try
	{
		auto reader = prep_cipher_slice->make_reader();
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

	auto reader = prep_cipher_slice->make_reader();
	ASSERT_EQ(reader.size(), slice_of_cipher_data.size());

	std::vector<char> result;
	reader.read_all(result);

	ASSERT_EQ(result.size(), reader.size());
	ASSERT_EQ(0, std::memcmp(result.data(), slice_of_cipher_data.data(), result.size()));

	kitchen->cancel_slicing();
}

TEST(kitchen_slicing, fetch_slice_of_sequential_reader)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device         = archive_diff::io::buffer::io_device;
	auto alphabet_reader = device::make_reader(alphabet_vector, device::size_kind::vector_capacity);
	std::shared_ptr<archive_diff::io::sequential::reader_factory> alphabet_factory =
		std::make_shared<sequential_reader_factory_of_reader>(alphabet_reader);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::sequential_reader_kind{alphabet_factory});

	for (size_t offset = 0; offset < c_letters_in_alphabet - 1; offset++)
	{
		for (size_t len_big_slice = 1; (offset + len_big_slice) < c_letters_in_alphabet; len_big_slice++)
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

			// We can technically do it, but we need the kitchen to begin slicing
			// so we expect this to throw an exception
			bool caught_exception{false};
			try
			{
				auto reader = prep_big_slice->make_reader();
			}
			catch (archive_diff::errors::user_exception &e)
			{
				if (e.get_error() == archive_diff::errors::error_code::diff_slicing_invalid_state)
				{
					caught_exception = true;
				}
			}

			ASSERT_TRUE(caught_exception);

			// Ok, now tell the kitchen to start slicing
			kitchen->resume_slicing();

			auto reader = prep_big_slice->make_reader();
			ASSERT_EQ(len_big_slice, reader.size());

			std::vector<char> result;
			reader.read_all(result);

			ASSERT_EQ(0, std::memcmp(result.data(), data_big_slice.data(), len_big_slice));

			kitchen->cancel_slicing();
		}
	}
}

TEST(kitchen_slicing, fetch_many_slices_of_sequential_reader)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device         = archive_diff::io::buffer::io_device;
	auto alphabet_reader = device::make_reader(alphabet_vector, device::size_kind::vector_capacity);
	std::shared_ptr<archive_diff::io::sequential::reader_factory> alphabet_factory =
		std::make_shared<sequential_reader_factory_of_reader>(alphabet_reader);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::sequential_reader_kind{alphabet_factory});

	for (size_t slice_gap = 1; slice_gap < 6; slice_gap++)
	{
		// Slice each letter individually
		std::map<size_t, item_definition> slice_items;

		auto kitchen = archive_diff::diffs::core::kitchen::create();
		archive_diff::diffs::recipes::basic::slice_recipe::recipe_template slice_recipe_template{};

		for (size_t i = 0; i < c_letters_in_alphabet; i += slice_gap)
		{
			slice_items.insert(
				std::pair{i, create_definition_from_data(std::string_view{alphabet_data.data() + i, 1})});
			ASSERT_FALSE(kitchen->can_fetch_item(slice_items[i]));

			std::vector<uint64_t> number_ingredients;
			number_ingredients.push_back(i);
			std::vector<archive_diff::diffs::core::item_definition> item_ingredients;
			item_ingredients.push_back(alphabet_item);

			auto slice_recipe =
				slice_recipe_template.create_recipe(slice_items[i], number_ingredients, item_ingredients);
			kitchen->add_recipe(slice_recipe);

			ASSERT_FALSE(kitchen->can_fetch_item(slice_items[i]));
		}

		kitchen->store_item(prep_alphabet);

		std::map<size_t, std::shared_ptr<archive_diff::diffs::core::prepared_item>> prepared_items;

		for (size_t i = 0; i < c_letters_in_alphabet; i += slice_gap)
		{
			kitchen->request_item(slice_items[i]);
		}

		ASSERT_TRUE(kitchen->process_requested_items());

		for (size_t i = 0; i < c_letters_in_alphabet; i += slice_gap)
		{
			prepared_items.insert(std::pair{i, kitchen->fetch_item(slice_items[i])});
		}

		kitchen->resume_slicing();

		for (size_t i = 0; i < c_letters_in_alphabet; i += slice_gap)
		{
			ASSERT_TRUE(prepared_items[i]->can_make_reader());
			auto reader = prepared_items[i]->make_reader();
			ASSERT_EQ(1, reader.size());

			std::vector<char> result;
			reader.read_all(result);

			ASSERT_EQ(1, result.size());

			ASSERT_EQ(result[0], 'a' + i);
		}

		kitchen->cancel_slicing();
	}
}

//
// most deep level is a sequential reader that contains: "abcdefghijklmnopqrstuvwxyz"
// next we have recipes yielding each of the letters singly
// next we spell out "the quick brown fox jumps over the lazy dog" using a chain of the the single parts (skipping
// spaces) lastly we define slices for "the" (one for each spot), "fox", "dog" and then "lazy" twice We use a custom
// reader for the alphabet here to synthetically make the deepest level slow
//
TEST(kitchen_slicing, tiered_slicing_and_chaining)
{
	auto alphabet_vector = std::make_shared<std::vector<char>>();
	alphabet_vector->reserve(c_letters_in_alphabet);
	std::memcpy(alphabet_vector->data(), c_alphabet, c_letters_in_alphabet);

	auto alphabet_data = std::string_view{alphabet_vector->data(), alphabet_vector->capacity()};

	item_definition alphabet_item{create_definition_from_data(alphabet_data)};

	using device         = archive_diff::io::buffer::io_device;
	auto alphabet_reader = device::make_reader(alphabet_vector, device::size_kind::vector_capacity);
	std::shared_ptr<archive_diff::io::sequential::reader_factory> alphabet_factory =
		std::make_shared<sequential_reader_factory_of_reader>(alphabet_reader);
	auto prep_alphabet = std::make_shared<archive_diff::diffs::core::prepared_item>(
		alphabet_item, archive_diff::diffs::core::prepared_item::sequential_reader_kind{alphabet_factory});

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

		ASSERT_FALSE(kitchen->can_fetch_item(slice_items[i]));
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

	ASSERT_FALSE(kitchen->can_fetch_item(test_string_item));

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
		// printf("%s: %s\n", part_string.c_str(), part_items.back().to_string().c_str());
	}

	std::vector<std::shared_ptr<archive_diff::diffs::core::prepared_item>> prepared_parts;

	for (size_t i = 0; i < parts_of_test_string->size(); i++)
	{
		auto &part_item = part_items[i];

		kitchen->request_item(part_item);
		ASSERT_TRUE(kitchen->process_requested_items());

		ASSERT_TRUE(kitchen->can_fetch_item(part_item));

		// printf("Going to fetch: %s\n", part_item.to_string().c_str());
		auto prep_part = kitchen->fetch_item(part_item);

		prepared_parts.push_back(prep_part);

		ASSERT_TRUE(prep_part->can_make_reader());

		bool caught_exception{false};
		try
		{
			auto reader = prep_part->make_reader();
		}
		catch (archive_diff::errors::user_exception &e)
		{
			if (e.get_error() == archive_diff::errors::error_code::diff_slicing_invalid_state)
			{
				caught_exception = true;
			}
		}

		ASSERT_TRUE(caught_exception);
	}

	kitchen->resume_slicing();

	for (size_t i = 0; i < parts_of_test_string->size(); i++)
	{
		auto &part_string = parts_of_test_string[i];
		auto &prep_part   = prepared_parts[i];

		auto reader = prep_part->make_reader();

		std::vector<char> result;
		reader.read_all(result);

		ASSERT_EQ(reader.size(), part_string.size());
		ASSERT_EQ(0, std::memcmp(result.data(), part_string.data(), part_string.size()));
	}

	kitchen->cancel_slicing();
}

#endif
