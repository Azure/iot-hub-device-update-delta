/**
 * @file inline_asset_recipe_tests.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "common.h"

#include <diffs/recipes/basic/inline_asset_recipe.h>

TEST(inline_asset_recipe, make_reader)
{
	auto test_inline_asset_data        = std::make_shared<std::vector<char>>();
	auto test_inline_asset_data_length = _countof(c_test_inline_asset_data);
	test_inline_asset_data->reserve(test_inline_asset_data_length);
	memcpy(const_cast<char *>(test_inline_asset_data->data()), c_test_inline_asset_data, test_inline_asset_data_length);

	auto reader      = std::make_shared<archive_diff::io::buffer::reader>(test_inline_asset_data);
	auto reader_view = archive_diff::io::reader_view{reader};

	test_apply_context context(reader_view);

	for (size_t offset = 0; offset < test_inline_asset_data_length - 1; offset++)
	{
		for (size_t length = 1; (length + offset) < test_inline_asset_data_length; length++)
		{
			auto expected_data = std::string_view{c_test_inline_asset_data + offset, length};

			archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
			hasher.hash_data(expected_data);

			archive_diff::diffs::blob_definition blobdef;
			blobdef.m_length   = length;
			auto expected_hash = hasher.get_hash();
			blobdef.m_hashes.push_back(expected_hash);

			archive_diff::diffs::recipes::basic::inline_asset_recipe recipe(blobdef);
			recipe.add_number_parameter(offset);

			auto recipe_result_reader = recipe.make_reader(&context);
			std::vector<char> recipe_result_data;
			recipe_result_data.resize(recipe_result_reader.size());
			recipe_result_reader.read(
				0, std::span{const_cast<char *>(recipe_result_data.data()), recipe_result_data.size()});

			ASSERT_EQ(recipe_result_data.size(), expected_data.size());
			ASSERT_EQ(0, memcmp(recipe_result_data.data(), expected_data.data(), expected_data.size()));

			archive_diff::hashing::hasher result_hasher(archive_diff::hashing::algorithm::sha256);
			result_hasher.hash_data(recipe_result_data);

			archive_diff::hashing::hash::verify_hashes_match(expected_hash, result_hasher.get_hash());
		}
	}
}

TEST(inline_asset_recipe, apply)
{
	auto test_inline_asset_data        = std::make_shared<std::vector<char>>();
	auto test_inline_asset_data_length = _countof(c_test_inline_asset_data);
	test_inline_asset_data->reserve(test_inline_asset_data_length);
	memcpy(const_cast<char *>(test_inline_asset_data->data()), c_test_inline_asset_data, test_inline_asset_data_length);

	auto reader      = std::make_shared<archive_diff::io::buffer::reader>(test_inline_asset_data);
	auto reader_view = archive_diff::io::reader_view{reader};

	test_apply_context context(reader_view);

	for (size_t offset = 0; offset < test_inline_asset_data_length - 1; offset++)
	{
		for (size_t length = 1; (length + offset) < test_inline_asset_data_length; length++)
		{
			auto expected_data = std::string_view{c_test_inline_asset_data + offset, length};

			archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
			hasher.hash_data(expected_data);

			archive_diff::diffs::blob_definition blobdef;
			blobdef.m_length   = length;
			auto expected_hash = hasher.get_hash();
			blobdef.m_hashes.push_back(expected_hash);

			archive_diff::diffs::recipes::basic::inline_asset_recipe recipe(blobdef);
			recipe.add_number_parameter(offset);

			// test apply_node::apply
			auto write_result_buffer = std::make_shared<std::vector<char>>();
			std::shared_ptr<archive_diff::io::writer> buffer_writer =
				std::make_shared<archive_diff::io::buffer::writer>(write_result_buffer);
			auto write_result_hasher =
				std::make_shared<archive_diff::hashing::hasher>(archive_diff::hashing::algorithm::sha256);
			archive_diff::io::hashed::hashed_sequential_writer hashed_writer(buffer_writer, write_result_hasher);

			recipe.apply(&context, hashed_writer);

			ASSERT_EQ(write_result_buffer->size(), expected_data.size());
			ASSERT_EQ(0, std::memcmp(write_result_buffer->data(), expected_data.data(), expected_data.size()));

			archive_diff::hashing::hasher result_hasher(archive_diff::hashing::algorithm::sha256);
			result_hasher.hash_data(*write_result_buffer);

			archive_diff::hashing::hash::verify_hashes_match(expected_hash, result_hasher.get_hash());
			archive_diff::hashing::hash::verify_hashes_match(expected_hash, write_result_hasher->get_hash());
		}
	}
}

TEST(inline_asset_recipe, no_inline_assets)
{
	nul_test_apply_context nul_context;
	auto test_inline_asset_data_length = _countof(c_test_inline_asset_data);

	for (size_t offset = 0; offset < test_inline_asset_data_length - 1; offset++)
	{
		for (size_t length = 1; (length + offset) < test_inline_asset_data_length; length++)
		{
			auto expected_data = std::string_view{c_test_inline_asset_data + offset, length};

			archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
			hasher.hash_data(expected_data);

			archive_diff::diffs::blob_definition blobdef;
			blobdef.m_length   = length;
			auto expected_hash = hasher.get_hash();
			blobdef.m_hashes.push_back(expected_hash);

			archive_diff::diffs::recipes::basic::inline_asset_recipe recipe(blobdef);
			recipe.add_number_parameter(offset);

			auto nul_recipe_result_reader = recipe.make_reader(&nul_context);
			ASSERT_EQ(0, nul_recipe_result_reader.size());
		}
	}
}

TEST(inline_asset_recipe, make_reader_buggy_result_bad_hash)
{
	auto test_inline_asset_data        = std::make_shared<std::vector<char>>();
	auto test_inline_asset_data_length = _countof(c_test_inline_asset_data);
	test_inline_asset_data->reserve(test_inline_asset_data_length);
	memcpy(const_cast<char *>(test_inline_asset_data->data()), c_test_inline_asset_data, test_inline_asset_data_length);

	auto reader      = std::make_shared<archive_diff::io::buffer::reader>(test_inline_asset_data);
	auto reader_view = archive_diff::io::reader_view{reader};

	buggy_test_apply_context buggy_context(reader_view);

	for (size_t offset = 0; offset < test_inline_asset_data_length - 1; offset++)
	{
		for (size_t length = 1; (length + offset) < test_inline_asset_data_length; length++)
		{
			auto expected_data = std::string_view{c_test_inline_asset_data + offset, length};

			archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
			hasher.hash_data(expected_data);

			archive_diff::diffs::blob_definition blobdef;
			blobdef.m_length   = length;
			auto expected_hash = hasher.get_hash();
			blobdef.m_hashes.push_back(expected_hash);

			archive_diff::diffs::recipes::basic::inline_asset_recipe recipe(blobdef);
			recipe.add_number_parameter(offset);

			auto buggy_recipe_result_reader = recipe.make_reader(&buggy_context);
			std::vector<char> buggy_recipe_result_data;
			buggy_recipe_result_data.resize(buggy_recipe_result_reader.size());
			buggy_recipe_result_reader.read(
				0, std::span{const_cast<char *>(buggy_recipe_result_data.data()), buggy_recipe_result_data.size()});

			if (buggy_recipe_result_data.size() == expected_data.size())
			{
				ASSERT_NE(0, memcmp(buggy_recipe_result_data.data(), expected_data.data(), expected_data.size()));
			}

			archive_diff::hashing::hasher buggy_result_hasher(archive_diff::hashing::algorithm::sha256);
			buggy_result_hasher.hash_data(buggy_recipe_result_data);

			bool caught_exception = false;
			try
			{
				archive_diff::hashing::hash::verify_hashes_match(expected_hash, buggy_result_hasher.get_hash());
			}
			catch (archive_diff::errors::user_exception &e)
			{
				ASSERT_EQ(archive_diff::errors::error_code::diff_verify_hash_failure, e.get_error());

				caught_exception = true;
			}
			ASSERT_TRUE(caught_exception);
		}
	}
}

TEST(inline_asset_recipe, apply_buggy_result_bad_hash)
{
	auto test_inline_asset_data        = std::make_shared<std::vector<char>>();
	auto test_inline_asset_data_length = _countof(c_test_inline_asset_data);
	test_inline_asset_data->reserve(test_inline_asset_data_length);
	memcpy(const_cast<char *>(test_inline_asset_data->data()), c_test_inline_asset_data, test_inline_asset_data_length);

	auto reader      = std::make_shared<archive_diff::io::buffer::reader>(test_inline_asset_data);
	auto reader_view = archive_diff::io::reader_view{reader};

	buggy_test_apply_context buggy_context(reader_view);

	for (size_t offset = 0; offset < test_inline_asset_data_length - 1; offset++)
	{
		for (size_t length = 1; (length + offset) < test_inline_asset_data_length; length++)
		{
			auto expected_data = std::string_view{c_test_inline_asset_data + offset, length};

			archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);
			hasher.hash_data(expected_data);

			archive_diff::diffs::blob_definition blobdef;
			blobdef.m_length   = length;
			auto expected_hash = hasher.get_hash();
			blobdef.m_hashes.push_back(expected_hash);

			archive_diff::diffs::recipes::basic::inline_asset_recipe recipe(blobdef);
			recipe.add_number_parameter(offset);

			// test apply_node::apply
			auto write_result_buffer = std::make_shared<std::vector<char>>();
			std::shared_ptr<archive_diff::io::writer> buffer_writer =
				std::make_shared<archive_diff::io::buffer::writer>(write_result_buffer);
			auto write_result_hasher =
				std::make_shared<archive_diff::hashing::hasher>(archive_diff::hashing::algorithm::sha256);

			archive_diff::io::hashed::hashed_sequential_writer hashed_writer(buffer_writer, write_result_hasher);

			recipe.apply(&buggy_context, hashed_writer);

			if (write_result_buffer->size() == expected_data.size())
			{
				ASSERT_NE(0, std::memcmp(write_result_buffer->data(), expected_data.data(), expected_data.size()));
			}

			archive_diff::hashing::hasher result_hasher(archive_diff::hashing::algorithm::sha256);
			result_hasher.hash_data(*write_result_buffer);

			bool caught_exception = false;
			try
			{
				archive_diff::hashing::hash::verify_hashes_match(expected_hash, result_hasher.get_hash());
			}
			catch (archive_diff::errors::user_exception &e)
			{
				ASSERT_EQ(archive_diff::errors::error_code::diff_verify_hash_failure, e.get_error());

				caught_exception = true;
			}
			ASSERT_TRUE(caught_exception);

			caught_exception = false;
			try
			{
				archive_diff::hashing::hash::verify_hashes_match(expected_hash, write_result_hasher->get_hash());
			}
			catch (archive_diff::errors::user_exception &e)
			{
				ASSERT_EQ(archive_diff::errors::error_code::diff_verify_hash_failure, e.get_error());

				caught_exception = true;
			}
			ASSERT_TRUE(caught_exception);
		}
	}
}
