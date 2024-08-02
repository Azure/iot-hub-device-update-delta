/**
 * @file test_item_definition.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <test_utility/gtest_includes.h>

#include <diffs/core/item_definition.h>
#include <hashing/hasher.h>

TEST(item_definition, operator_less_than_overload_starting_equivalent_items)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	// two items with no hashes (and same name) at construction and equal length should
	// always return false when comparing.
	std::pair<item_definition, item_definition> test_data[] = {
		{{}, {}},
		{{0}, {0}},
		{{1000}, {1000}},
	};

	for (auto test_case : test_data)
	{
		auto lhs = test_case.first;
		auto rhs = test_case.second;

		ASSERT_FALSE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		lhs = lhs.with_hash(hash1);
		ASSERT_FALSE(lhs < rhs);
		ASSERT_TRUE(rhs < lhs);

		rhs = rhs.with_hash(hash1);
		ASSERT_FALSE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		lhs = lhs.with_hash(hash2);
		ASSERT_FALSE(lhs < rhs);
		ASSERT_TRUE(rhs < lhs);

		rhs = rhs.with_hash(hash2);
		ASSERT_FALSE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);
	}
}

TEST(item_definition, operator_less_than_overload_starting_differing_sized_items)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	// two items with no hashes (and same name) at construction, but first is smaller than second
	// always return false when comparing.
	for (int i = 0; i < 2; i++)
	{
		auto lhs = item_definition{0};
		auto rhs = item_definition{1};

		if (i == 1)
		{
			lhs = lhs.with_name("name");
			rhs = rhs.with_name("name");
		}

		ASSERT_TRUE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		lhs = lhs.with_hash(hash1);
		ASSERT_TRUE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		rhs = rhs.with_hash(hash1);
		ASSERT_TRUE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		lhs = lhs.with_hash(hash2);
		ASSERT_TRUE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);

		rhs = rhs.with_hash(hash2);
		ASSERT_TRUE(lhs < rhs);
		ASSERT_FALSE(rhs < lhs);
	}
}

TEST(item_definition, match_starting_equivalent_items)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	// two items with no hashes (and same name) at construction and equal length should
	// always return false when comparing.
	std::pair<item_definition, item_definition> test_data[] = {
		{{}, {}},
		{{0}, {0}},
		{{1000}, {1000}},
	};

	for (auto test_case : test_data)
	{
		for (int i = 0; i < 2; i++)
		{
			auto lhs = test_case.first;
			auto rhs = test_case.second;

			ASSERT_EQ(item_definition::match_result::uncertain, lhs.match(rhs));

			lhs = lhs.with_hash(hash1);
			ASSERT_EQ(item_definition::match_result::uncertain, lhs.match(rhs));

			rhs = rhs.with_hash(hash1);
			ASSERT_EQ(item_definition::match_result::match, lhs.match(rhs));

			lhs = lhs.with_hash(hash2);
			ASSERT_EQ(item_definition::match_result::match, lhs.match(rhs));

			rhs = rhs.with_hash(hash2);
			ASSERT_EQ(item_definition::match_result::match, lhs.match(rhs));
		}
	}
}

TEST(item_definition, match_starting_differing_sized_items)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	// two items with no hashes (and same name) at construction, but first is smaller than second
	// always return false when comparing.
	for (int i = 0; i < 2; i++)
	{
		item_definition lhs{0};
		item_definition rhs{1000};

		if (i == 1)
		{
			lhs = lhs.with_name("name");
			rhs = rhs.with_name("name");
		}

		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		lhs = lhs.with_hash(hash1);
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		rhs = rhs.with_hash(hash1);
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		lhs = lhs.with_hash(hash2);
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		rhs = rhs.with_hash(hash2);
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		lhs = lhs.with_name("name2");
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));

		rhs = rhs.with_name("name2");
		ASSERT_EQ(item_definition::match_result::no_match, lhs.match(rhs));
	}
}

TEST(item_definition, has_matching_hash)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	item_definition test_data[] = {
		{},
		{0},
		{1000},
	};

	for (auto &test_case : test_data)
	{
		item_definition item = test_case;

		ASSERT_EQ(false, item.has_matching_hash(hash2));
		ASSERT_EQ(false, item.has_matching_hash(hash1));

		item = item.with_hash(hash1);
		ASSERT_EQ(true, item.has_matching_hash(hash1));
		ASSERT_EQ(false, item.has_matching_hash(hash2));

		item = item.with_hash(hash2);
		ASSERT_EQ(true, item.has_matching_hash(hash1));
		ASSERT_EQ(true, item.has_matching_hash(hash2));

		item = item.with_name("name2");
		ASSERT_EQ(true, item.has_matching_hash(hash1));
		ASSERT_EQ(true, item.has_matching_hash(hash2));
	}
}

// Make items with each constructor and try verify that they have the correct size
// Add a hash and verify it is still true
// Add a name and verify it is still true
TEST(item_definition, size)
{
	using item_definition = archive_diff::diffs::core::item_definition;

	archive_diff::hashing::hasher sha256(archive_diff::hashing::algorithm::sha256);
	auto hash1 = sha256.get_hash();

	archive_diff::hashing::hasher md5(archive_diff::hashing::algorithm::md5);
	auto hash2 = md5.get_hash();

	std::pair<item_definition, uint64_t> test_data[] = {
		{{}, 0},
		{{0}, 0},
		{{1000}, 1000},
	};

	for (auto &test_case : test_data)
	{
		for (int i = 0; i < 4; i++)
		{
			auto item          = test_case.first;
			auto expected_size = test_case.second;

			switch (i)
			{
			case 0:
				break;
			case 1:
				item = item.with_hash(hash1);
				break;
			case 2:
				item = item.with_name("name1");
				break;
			case 3:
				item = item.with_hash(hash1).with_name("name1");
				break;
			}

			ASSERT_EQ(expected_size, item.size());

			item = item.with_hash(hash1);
			ASSERT_EQ(expected_size, item.size());

			item = item.with_hash(hash2);
			ASSERT_EQ(expected_size, item.size());

			item = item.with_name("name2");
			ASSERT_EQ(expected_size, item.size());
		}
	}
}
