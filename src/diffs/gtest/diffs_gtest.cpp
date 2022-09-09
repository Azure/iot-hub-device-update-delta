/**
 * @file diffs_gtest.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include <cstdint>
#include <fstream>
#include <istream>
#include <filesystem>
#include <thread>
#include <chrono>

#ifdef WIN32
	#define EXECUTION_SUPPORTED
#endif

#ifdef EXECUTION_SUPPORTED
	#include <execution>
#elif defined(_GLIBCXX_PARALLEL)
	#include <parallel/algorithm>
#endif

namespace fs = std::filesystem;

#include "diff.h"
#include "apply_context.h"
#include "archive_item.h"
#include "recipe.h"
#include "blob_cache.h"

#include "user_exception.h"

#include "hash_utility.h"

#include "zstd_compression_writer.h"
#include "zstd_decompression_reader.h"
#include "binary_file_readerwriter.h"
#include "binary_file_writer.h"
#include "wrapped_writer_sequential_hashed_writer.h"
#include "stored_blob_reader.h"
#include "binary_file_reader.h"

#include "recipe_names.h"

#include "gtest/gtest.h"
using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

fs::path test_data_root;
fs::path zstd_compress_file;

fs::path get_data_file(const fs::path file)
{
	auto path = test_data_root / file;
	return path;
}

const fs::path c_source_remainder_compressed   = "source/remainder.dat.deflate";
const fs::path c_source_remainder_uncompressed = "source/remainder.dat";
const fs::path c_source_boot_file_compressed   = "source/boot.zst";
const fs::path c_source_archive                = "source/source.swu";

int main(int argc, char **argv)
{
	printf("argc: %d\n", argc);
	for (int i = 0; i < argc; i++)
	{
		printf("argv[%d] = %s\n", i, argv[i]);
	}
	if (argc > 4 && (strcmp(argv[1], "--test_data_root") == 0) && (strcmp(argv[3], "--zstd_compress_file") == 0))
	{
		test_data_root     = argv[2];
		zstd_compress_file = argv[4];

		printf("test_data_root: %s\n", test_data_root.string().c_str());
		printf("zstd_compress_file: %s\n", zstd_compress_file.string().c_str());
	}
	else
	{
		printf(
			"Must specify test data root with --test_data_root <path> and binary path with --zstd_compress_file "
			"<path>\n");
		return 1;
	}

	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

void populate_readers(diffs::blob_cache &cache, io_utility::reader &reader)
{
	using namespace std::chrono_literals;
	cache.populate_from_reader(&reader);
	std::this_thread::sleep_for(3s);
}

void make_target_file_path(std::string test_name, fs::path *target_path)
{
	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;
	*target_path           = base_test_working / "target";
}

void make_working_directory_path(std::string test_name, fs::path *working_path)
{
	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;

	*working_path = base_test_working / "working";
}

void make_source_and_target_files(
	std::string test_name,
	gsl::span<const char> source_data,
	fs::path *source_path,
	fs::path *target_path,
	fs::path *working_path)
{
	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;
	*source_path           = base_test_working / "source";
	make_working_directory_path(test_name, working_path);

	fs::remove_all(*working_path);
	fs::create_directories(*working_path);

	make_target_file_path(test_name, target_path);

	std::ofstream source_ostream(*source_path, std::ios::out | std::ios::binary);
	source_ostream.write(source_data.data(), source_data.size());
	source_ostream.close();
}

static std::vector<char> get_hash(const char *data, size_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	std::string_view view{data, length};
	hasher.hash_data(view);

	return hasher.get_hash_binary();
}

static std::vector<char> get_hash(io_utility::reader *reader, uint64_t offset, uint64_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	std::vector<char> buffer;
	buffer.resize(static_cast<size_t>(32) * 1024);

	auto remaining = length;

	auto current_offset = offset;

	while (remaining > 0)
	{
		size_t to_read = std::min(static_cast<size_t>(buffer.capacity()), static_cast<size_t>(remaining));

		reader->read(current_offset, gsl::span<char>{buffer.data(), to_read});

		hasher.hash_data(std::string_view{buffer.data(), static_cast<size_t>(to_read)});

		current_offset += to_read;
		remaining -= to_read;
	}

	return hasher.get_hash_binary();
}

static std::vector<char> get_hash(fs::path path, uint64_t offset, uint64_t length)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	io_utility::binary_file_reader reader(path.string());

	return get_hash(&reader, offset, length);
}

static std::vector<char> get_hash(fs::path path)
{
	auto file_size = fs::file_size(path);

	return get_hash(path, 0, file_size);
}

static std::vector<std::vector<char>> get_hashes(fs::path path)
{
	std::vector<std::vector<char>> hashes;

	auto file_size      = fs::file_size(path);
	const size_t length = static_cast<size_t>(1024 * 1024);

	for (size_t offset = 0; offset < file_size; offset += length)
	{
		auto actual_length = std::min<size_t>(file_size - offset, length);
		hashes.emplace_back(get_hash(path, offset, actual_length));
	}

	return hashes;
}

static void create_file(fs::path file, std::vector<char> &data)
{
	std::ofstream file_stream(file, std::ios::binary | std::ios::out);

	file_stream.write(data.data(), data.size());
}

bool FilesAreEqual(fs::path path1, fs::path path2)
{
	auto size1 = fs::file_size(path1);
	auto size2 = fs::file_size(path2);

	if (size1 != size2)
	{
		return false;
	}

	std::ifstream file1(path1, std::ios::in | std::ios::binary);
	std::ifstream file2(path2, std::ios::in | std::ios::binary);

	auto remaining = size1;

	std::vector<char> read_buffer1;
	std::vector<char> read_buffer2;

	const size_t read_block_size = static_cast<size_t>(32 * 1024);
	read_buffer1.reserve(read_block_size);
	read_buffer2.reserve(read_block_size);

	while (remaining)
	{
		size_t to_read = std::min(static_cast<size_t>(read_block_size), static_cast<size_t>(remaining));

		file1.read(read_buffer1.data(), to_read);
		file2.read(read_buffer2.data(), to_read);

		if (0 != memcmp(read_buffer1.data(), read_buffer2.data(), to_read))
		{
			return false;
		}

		remaining -= to_read;
	}

	return true;
}

#define VERIFY_FILES_ARE_EQUAL(file1, file2)                                            \
	{                                                                                   \
		printf("Comparing %s to %s\n", file1.string().c_str(), file2.string().c_str()); \
		ASSERT_TRUE(FilesAreEqual(file1, file2));                                       \
	}

TEST(recipe_copy_source, basic)
{
	diffs::recipe_host recipe_host;

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24
	  [  ]  [     A     ]
	*/
	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	//  A = {0, 1, 2, 3, 4}

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
	  [     A     ]  [     B     ]
	*/
	const char target_data[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 3, 4}
	// B = {0, 1, 2, 3, 4}
	const uint64_t source_a_offset = 2;
	const uint64_t source_a_length = 5;

	const uint64_t target_a_offset = 0;
	const uint64_t target_a_length = 5;

	fs::path source_path;
	fs::path target_path;
	fs::path working_folder;

	make_source_and_target_files(
		"TestApply_CopySource_And_Copy",
		gsl::span<const char>{source_data, sizeof(source_data)},
		&source_path,
		&target_path,
		&working_folder);

	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);

	{
		diffs::blob_cache blob_cache;

		diffs::diff diff;

		io_utility::binary_file_readerwriter diff_reader((working_folder / "empty.diff").string());

		diffs::apply_context::root_context_parameters params;
		params.m_diff           = &diff;
		params.m_diff_reader    = &diff_reader;
		params.m_source_file    = source_path.string();
		params.m_target_file    = target_path.string();
		params.m_blob_cache     = &blob_cache;

		diffs::apply_context context = diffs::apply_context::root_context(params);

		// Copy Bytes From Source to Target (Source A -> Target A)

		diffs::archive_item copy_source_chunk{
			target_a_offset,
			target_a_length,
			diffs::archive_item_type::chunk,
			diffs::hash_type::Sha256,
			target_a_hash.data(),
			target_a_hash.size()};

		auto copy_source_recipe = copy_source_chunk.create_recipe(&recipe_host, diffs::copy_source_recipe_name);
		copy_source_recipe->add_number_parameter(source_a_offset);
		copy_source_recipe->add_number_parameter(source_a_length);

		copy_source_chunk.apply(context);
	}

	std::ifstream target_istream(target_path, std::ios::binary | std::ios::in);

	char target_a_data[target_a_length]{0};
	target_istream.read(target_a_data, target_a_length);

	ASSERT_EQ(0, memcmp(target_a_data, &target_data[target_a_offset], target_a_length));
}

TEST(recipe_copy_source, with_recipe_copy)
{
	diffs::recipe_host recipe_host;

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24
	  [  ]  [     A     ]
	*/
	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	//  A = {0, 1, 2, 3, 4}

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
	  [     A     ]  [     B     ]
	*/
	const char target_data[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 3, 4}
	// B = {0, 1, 2, 3, 4}
	const uint64_t source_a_offset = 2;
	const uint64_t source_a_length = 5;

	const uint64_t target_a_offset = 0;
	const uint64_t target_a_length = 5;
	const uint64_t target_b_offset = target_a_offset + target_a_length;
	const uint64_t target_b_length = 5;

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / "TestApply_CopySource_And_Copy";
	auto source_path       = base_test_working / "source";
	auto target_path       = base_test_working / "target";

	fs::create_directories(base_test_working);
	auto working_folder = base_test_working / "working";
	fs::create_directories(working_folder);

	std::ofstream source_ostream(source_path, std::ios::out | std::ios::binary);
	source_ostream.write(source_data, sizeof(source_data));
	source_ostream.close();

	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);
	auto target_b_hash = get_hash(&target_data[target_b_offset], target_b_length);

	{
		diffs::blob_cache blob_cache;

		diffs::diff diff;

		io_utility::binary_file_readerwriter diff_reader((base_test_working / "empty.diff").string());

		diffs::apply_context::root_context_parameters params;
		params.m_diff           = &diff;
		params.m_diff_reader    = &diff_reader;
		params.m_source_file    = source_path.string();
		params.m_target_file    = target_path.string();
		params.m_blob_cache     = &blob_cache;

		diffs::apply_context context = diffs::apply_context::root_context(params);

		// Copy Bytes From Source to Target (Source A -> Target A)

		diffs::archive_item copy_source_chunk{
			target_a_offset,
			target_a_length,
			diffs::archive_item_type::chunk,
			diffs::hash_type::Sha256,
			target_a_hash.data(),
			target_a_hash.size()};

		auto copy_source_recipe =
			copy_source_chunk.create_recipe(&recipe_host, diffs::copy_source_recipe_name);
		copy_source_recipe->add_number_parameter(source_a_offset);
		copy_source_recipe->add_number_parameter(source_a_length);

		// Copy Bytes From Source to Target (Target A -> Target B)

		diffs::archive_item copy_chunk{
			target_b_offset,
			target_b_length,
			diffs::archive_item_type::chunk,
			diffs::hash_type::Sha256,
			target_b_hash.data(),
			target_b_hash.size()};

		auto copy_recipe = copy_chunk.create_recipe(&recipe_host, diffs::copy_recipe_name);

		(void)copy_recipe->add_archive_item_parameter(
			diffs::archive_item_type::chunk,
			target_a_offset,
			target_a_length,
			diffs::hash_type::Sha256,
			target_a_hash.data(),
			target_a_hash.size());

		copy_source_chunk.apply(context);
		copy_chunk.apply(context);
	}

	std::ifstream target_istream(target_path, std::ios::binary | std::ios::in);

	char target_a_data[target_a_length]{0};
	target_istream.read(target_a_data, target_a_length);

	char target_b_data[target_b_length]{0};
	target_istream.read(target_b_data, target_b_length);

	ASSERT_EQ(0, memcmp(target_a_data, &target_data[target_a_offset], target_a_length));
	ASSERT_EQ(0, memcmp(target_b_data, &target_data[target_b_offset], target_b_length));
}

TEST(recipe_copy, bad_hash)
{
	diffs::recipe_host recipe_host;

	/*
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24
  [  ]  [     A     ]
*/
	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	//  A = {0, 1, 2, 3, 4}

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
	  [     A     ]  [     B     ]
	*/
	const char target_data[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 3, 4}
	// B = {0, 1, 2, 3, 4}
	const uint64_t source_a_offset = 2;
	const uint64_t source_a_length = 5;

	const uint64_t target_a_offset = 0;
	const uint64_t target_a_length = 5;
	const uint64_t target_b_offset = target_a_offset + target_a_length;
	const uint64_t target_b_length = 5;

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / "TestApply_BadCopy";
	auto source_path       = base_test_working / "source";
	auto target_path       = base_test_working / "target";

	fs::create_directories(base_test_working);
	auto working_folder = base_test_working / "working";
	fs::remove_all(base_test_working);
	fs::create_directories(base_test_working);

	std::ofstream source_ostream(source_path, std::ios::out | std::ios::binary);
	source_ostream.write(source_data, sizeof(source_data));
	source_ostream.close();

	/* Now try to do a bad copy. Copy some of A, but not all of it. */

	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);
	auto target_b_hash = get_hash(&target_data[target_b_offset], target_b_length);

	diffs::blob_cache blob_cache;

	diffs::diff diff;

	io_utility::binary_file_readerwriter diff_reader((base_test_working / "empty.diff").string());

	diffs::apply_context::root_context_parameters params;
	params.m_diff           = &diff;
	params.m_diff_reader    = &diff_reader;
	params.m_source_file    = source_path.string();
	params.m_target_file    = target_path.string();
	params.m_blob_cache     = &blob_cache;

	diffs::apply_context context = diffs::apply_context::root_context(params);

	// Copy Bytes From Source to Target (Source A -> Target A)

	diffs::archive_item copy_source_chunk{
		target_a_offset,
		target_a_length,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		target_a_hash.data(),
		target_a_hash.size()};

	auto copy_source_recipe =
		copy_source_chunk.create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(source_a_offset);
	copy_source_recipe->add_number_parameter(source_a_length);

	diffs::archive_item bad_copy_chunk{
		target_b_offset,
		target_b_length,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		target_b_hash.data(),
		target_b_hash.size()};

	auto bad_copy_recipe = bad_copy_chunk.create_recipe(&recipe_host, diffs::copy_recipe_name);

	(void)bad_copy_recipe->add_archive_item_parameter(
		diffs::archive_item_type::chunk,
		target_a_offset,
		target_a_length - 1,
		diffs::hash_type::Sha256,
		target_a_hash.data(),
		target_a_hash.size());

	copy_source_chunk.apply(context);

	bool caughtException = false;
	try
	{
		bad_copy_chunk.apply(context);
	}
	catch (error_utility::user_exception &e)
	{
		ASSERT_EQ(error_utility::error_code::diff_verify_hash_failure, e.get_error());
        printf("Exception Message: %s\n", e.get_message());
		caughtException = true;
	}
	ASSERT_TRUE(caughtException);
}

TEST(recipe_region, with_copy_source)
{
	diffs::recipe_host recipe_host;
	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24
	  [       A               ]
	*/
	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 0, 0, 1, 2, 3, 4, 4, 5}

	/*
	  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
	  [     A     ]
	*/
	const char target_data[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 3, 4}

	const uint64_t source_a_offset = 0;
	const uint64_t source_a_length = 9;

	const uint64_t target_a_offset = 0;
	const uint64_t target_a_length = 5;

	/* Copy part of Source A to make Target A */

	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);

	diffs::archive_item target_a_chunk{
		target_a_offset,
		target_a_length,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		target_a_hash.data(),
		target_a_hash.size()};

	auto region_recipe = target_a_chunk.create_recipe(&recipe_host, diffs::region_recipe_name);

	auto source_a_hash = get_hash(&source_data[source_a_offset], source_a_length);

	auto region_blob_param = region_recipe->add_archive_item_parameter(
		diffs::archive_item_type::blob,
		source_a_offset,
		source_a_length,
		diffs::hash_type::Sha256,
		source_a_hash.data(),
		source_a_hash.size());
	auto region_blob = region_blob_param->get_archive_item_value();

	auto copy_source_recipe = region_blob->create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(source_a_offset);
	copy_source_recipe->add_number_parameter(source_a_length);

	region_recipe->add_number_parameter(2);
	region_recipe->add_number_parameter(target_a_length);

	fs::path source_path;
	fs::path target_path;
	fs::path working_folder;

	make_source_and_target_files(
		"TestApply_TestRegionOfCopySource",
		std::string_view{source_data, sizeof(source_data)},
		&source_path,
		&target_path,
		&working_folder);

	// use scope to cause the context to flush its output
	{
		diffs::blob_cache blob_cache;

		diffs::diff diff;

		auto temp_path = fs::temp_directory_path() / "TestApply_TestRegionOfCopySource";
		fs::remove_all(temp_path);
		fs::create_directories(temp_path);

		io_utility::binary_file_readerwriter diff_reader((temp_path / "empty.diff").string());

		diffs::apply_context::root_context_parameters params;
		params.m_diff           = &diff;
		params.m_diff_reader    = &diff_reader;
		params.m_source_file    = source_path.string();
		params.m_target_file    = target_path.string();
		params.m_blob_cache     = &blob_cache;

		diffs::apply_context context = diffs::apply_context::root_context(params);

		target_a_chunk.apply(context);
	}

	std::ifstream target_istream(target_path, std::ios::binary | std::ios::in);

	char target_a_data[target_a_length];
	target_istream.read(target_a_data, target_a_length);

	ASSERT_EQ(0, memcmp(target_a_data, &target_data[target_a_offset], target_a_length));
}

void add_copy_source_param(diffs::recipe_host* recipe_host, diffs::recipe *recipe, const char *source_data, uint64_t offset, uint64_t length)
{
	auto hash = get_hash(&source_data[offset], length);

	auto concat_param3 = recipe->add_archive_item_parameter(
		diffs::archive_item_type::blob, offset, length, diffs::hash_type::Sha256, hash.data(), hash.size());
	auto concat_blob3 = concat_param3->get_archive_item_value();
	auto copy_recipe3 = concat_blob3->create_recipe(recipe_host, diffs::copy_source_recipe_name);
	copy_recipe3->add_number_parameter(offset);
	copy_recipe3->add_number_parameter(length);
}

static void modify_source(std::vector<char> &data, size_t size, size_t mod1, size_t mod2)
{
	data.resize(size);

	for (size_t i = 0; i < data.size(); i++)
	{
		if (i % mod1 == 0)
		{
			data[i] = 4;
		}

		if (i % mod2 == 0)
		{
			data[i] |= 33;
		}
	}
}

static void modify_target(std::vector<char> &data, size_t size, size_t mod1, size_t mod2)
{
	data.resize(size);

	for (size_t i = 0; i < data.size(); i++)
	{
		data[i] = 1;

		if (i % mod1 == 0)
		{
			data[i] = 4;
		}

		if (i % mod2 == 0)
		{
			data[i] |= 33;
		}

		if (i % 10 == 0)
		{
			data[i] ^= 0xFEF1F0F3;
		}
	}
}

static void write_data_to_file(fs::path path, std::string_view data)
{
	io_utility::binary_file_writer writer(path.string());

	writer.write(0, data);
}

static void write_diff(diffs::diff *diff, fs::path diff_path, fs::path inline_assets_path, fs::path remainder_path)
{
	io_utility::binary_file_writer diff_writer(diff_path.string());
	io_utility::binary_file_reader inline_assets_reader(inline_assets_path.string());
	io_utility::binary_file_reader remainder_reader(remainder_path.string());

	diffs::diff_writer_context writer_context{&diff_writer, &inline_assets_reader, &remainder_reader};
	diff->write(writer_context);
}

using pfn_add_to_diff = void (*)(diffs::diff *diff, void *data);

struct recipe_delta_context
{
	const char* recipe_type_name;

	fs::path source_path;
	fs::path target_path;
	fs::path delta_path;
};

static void add_delta_to_diff(diffs::diff *diff, void *data)
{
	recipe_delta_context *context = reinterpret_cast<recipe_delta_context *>(data);
	diffs::recipe_host recipe_host;

	auto target_size = fs::file_size(context->target_path);

	auto target_hash = get_hash(context->target_path);

	auto target_chunk =
		diff->add_chunk(0, target_size, diffs::hash_type::Sha256, target_hash.data(), target_hash.size());

	auto recipe = target_chunk->create_recipe(&recipe_host, context->recipe_type_name);
		
	auto delta_size = fs::file_size(context->delta_path);
	auto delta_hash = get_hash(context->delta_path);

	auto delta_param = recipe->add_archive_item_parameter(
		diffs::archive_item_type::blob, 0, delta_size, diffs::hash_type::Sha256, delta_hash.data(), delta_hash.size());
	auto delta_blob = delta_param->get_archive_item_value();

	auto inline_asset_recipe = delta_blob->create_recipe(&recipe_host, diffs::inline_asset_recipe_name);
	inline_asset_recipe->add_number_parameter(0u);
	inline_asset_recipe->add_number_parameter(delta_size);

	auto source_size = fs::file_size(context->source_path);
	auto source_hash = get_hash(context->source_path);

	auto source_param = recipe->add_archive_item_parameter(
		diffs::archive_item_type::payload,
		0,
		source_size,
		diffs::hash_type::Sha256,
		source_hash.data(),
		source_hash.size());
	auto source_blob        = source_param->get_archive_item_value();
	auto copy_source_recipe = source_blob->create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(0); // offset
}

void create_diff(
	fs::path source_path,
	fs::path target_path,
	pfn_add_to_diff pfn,
	void *data,
	fs::path diff_path,
	fs::path inline_assets_path,
	fs::path remainder_path,
	fs::path remainder_path_deflate)
{
	printf("Creating diff from %s to %s\n", source_path.string().c_str(), target_path.string().c_str());
	printf("diff_path: %s\n", diff_path.string().c_str());
	printf("inline_assets_path: %s\n", inline_assets_path.string().c_str());
	printf("remainder_path: %s\n", remainder_path.string().c_str());
	printf("remainder_path_deflate: %s\n", remainder_path_deflate.string().c_str());

	auto target_hash = get_hash(target_path);

	diffs::diff diff;

	diff.set_target_hash(diffs::hash_type::Sha256, target_hash.data(), target_hash.size());
	diff.set_target_size(fs::file_size(target_path));

	diff.set_remainder_sizes(fs::file_size(remainder_path), fs::file_size(remainder_path_deflate));

	pfn(&diff, data);

	write_diff(&diff, diff_path, inline_assets_path, remainder_path_deflate);
}

void apply_diff(fs::path source_path, fs::path diff_path, fs::path target_path, fs::path working_folder)
{
	io_utility::binary_file_reader diff_reader(diff_path.string());
	diffs::diff diff_from_file(&diff_reader);

	diffs::blob_cache blob_cache;

	diffs::apply_context::root_context_parameters params;
	params.m_diff = &diff_from_file;

	params.m_diff_reader    = &diff_reader;
	params.m_source_file    = source_path.string();
	params.m_target_file    = target_path.string();
	params.m_blob_cache     = &blob_cache;

	diffs::apply_context apply_context = diffs::apply_context::root_context(params);

	diff_from_file.apply(apply_context);
}

// TEST_METHOD(recipe_bsdiff, basic)
//{
//	recipe_delta_context context;

//	std::vector<char> source_data;
//	std::vector<char> target_data;

//	source_data.resize(5000);
//	target_data.resize(5300);

//	modify_target(target_data, 5300, 37, 13);

//	auto test_name = "Recipe_Bsdiff";

//	auto temp              = fs::temp_directory_path();
//	auto base_test_working = temp / "diffs_test" / test_name;

//	fs::remove_all(base_test_working);
//	fs::create_directories(base_test_working);

//	context.source_path = base_test_working / "source";
//	context.target_path = base_test_working / "target";
//	context.delta_path  = base_test_working / "delta";

//	write_data_to_file(context.source_path, std::string_view{source_data.data(), source_data.size()});
//	write_data_to_file(context.target_path, std::string_view{target_data.data(), target_data.size()});

//	std::string command_line = "bsdiff " + context.source_path.string() + " " + context.target_path.string() + " "
//	                         + context.delta_path.string();
//	system(command_line.c_str());

//	ASSERT_TRUE(fs::exists(context.delta_path));

//	auto diff_path          = base_test_working / "diff";
//	auto remainder_path     = base_test_working / "remainder";
//	auto inline_assets_path = context.delta_path;

//	std::fstream remainder_stream(remainder_path, std::ios::binary | std::ios::out);
//	remainder_stream.close();

//	context.type = diffs::recipe_type::bsdiff_delta;

//	create_diff(
//		context.source_path,
//		context.target_path,
//		add_delta_to_diff,
//		&context,
//		diff_path,
//		inline_assets_path,
//		remainder_path,
//		remainder_path);

//	auto target_path_applied = context.target_path.string() + ".applied";
//	auto working_folder      = base_test_working / "working";
//	apply_diff(context.source_path, diff_path, target_path_applied, working_folder);

//	ASSERT_TRUE(FilesAreEqual(context.target_path, target_path_applied));
//}

struct inline_asset_context
{
	fs::path source_path;
	fs::path target_path;
};

static void add_inline_asset_to_diff(diffs::diff *diff, std::vector<char> &file_data)
{
	auto file_hash = get_hash(file_data.data(), file_data.size());

	auto target_chunk =
		diff->add_chunk(0, file_data.size(), diffs::hash_type::Sha256, file_hash.data(), file_hash.size());

	auto recipe = target_chunk->create_recipe(diff, diffs::inline_asset_recipe_name);

	recipe->add_number_parameter(0u);
	recipe->add_number_parameter(static_cast<uint64_t>(file_data.size()));
}

static void add_inline_asset_to_diff(diffs::diff *diff, void *data)
{
	inline_asset_context *context = reinterpret_cast<inline_asset_context *>(data);

	io_utility::binary_file_reader reader(context->target_path.string());

	std::vector<char> target_data;
	target_data.resize(reader.size());

	reader.read(0, gsl::span<char>{target_data.data(), target_data.size()});

	add_inline_asset_to_diff(diff, target_data);
}

static void generate_source_and_target_files(
	std::vector<char> &source_data,
	std::vector<char> &target_data,
	size_t source_size,
	size_t target_size,
	size_t mod1,
	size_t mod2,
	fs::path source_path,
	fs::path target_path)
{
	modify_source(source_data, 5000, mod1, mod2);
	modify_target(target_data, 5300, mod1, mod2);

	write_data_to_file(source_path, std::string_view{source_data.data(), source_data.size()});
	write_data_to_file(target_path, std::string_view{target_data.data(), target_data.size()});
}

TEST(recipe_inline_asset, basic)
{
	auto test_name = "Recipe_InlineAsset";

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;

	fs::remove_all(base_test_working);
	fs::create_directories(base_test_working);

	inline_asset_context context;

	context.source_path = base_test_working / "source";
	context.target_path = base_test_working / "target";

	std::vector<char> source_data;
	std::vector<char> target_data;
	generate_source_and_target_files(
		source_data, target_data, 5000, 53000, 37, 13, context.source_path, context.target_path);

	auto diff_path          = base_test_working / "diff";
	auto remainder_path     = base_test_working / "remainder";
	auto& inline_assets_path = context.target_path;

	std::fstream remainder_stream(remainder_path, std::ios::binary | std::ios::out);
	remainder_stream.close();

	create_diff(
		context.source_path,
		context.target_path,
		add_inline_asset_to_diff,
		&context,
		diff_path,
		inline_assets_path,
		remainder_path,
		remainder_path);

	auto target_path_applied = fs::path(context.target_path.string() + ".applied");
	auto working_folder      = base_test_working / "working";
	apply_diff(context.source_path, diff_path, target_path_applied, working_folder);

	VERIFY_FILES_ARE_EQUAL(context.target_path, target_path_applied);
}

struct remainder_chunk_context
{
	fs::path remainder_path;
	fs::path remainder_path_deflate;
};

static void add_remainder_chunk_to_diff(diffs::diff *diff, void *data)
{
	remainder_chunk_context *context = reinterpret_cast<remainder_chunk_context *>(data);

	auto target_hash = get_hash(context->remainder_path);

	auto target_chunk = diff->add_chunk(
		0, fs::file_size(context->remainder_path), diffs::hash_type::Sha256, target_hash.data(), target_hash.size());

	auto recipe = target_chunk->create_recipe(diff, diffs::remainder_chunk_recipe_name);
}

TEST(recipe_remainder_chunk, basic)
{
	remainder_chunk_context context;

	auto test_name = "Recipe_RemainderChunk";

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;

	fs::remove_all(base_test_working);
	fs::create_directories(base_test_working);

	auto source_path = base_test_working / "source";
	std::fstream source_stream(source_path, std::ios::binary | std::ios::out);
	source_stream.close();

	auto diff_path = base_test_working / "diff";

	auto inline_assets_path = base_test_working / "inline_assets";

	std::fstream inline_assets_stream(inline_assets_path, std::ios::binary | std::ios::out);
	inline_assets_stream.close();

	context.remainder_path = (fs::current_path() / get_data_file(c_source_remainder_uncompressed)).lexically_normal();
	context.remainder_path_deflate =
		(fs::current_path() / get_data_file(c_source_remainder_compressed)).lexically_normal();

	create_diff(
		source_path,
		context.remainder_path,
		add_remainder_chunk_to_diff,
		&context,
		diff_path,
		inline_assets_path,
		context.remainder_path,
		context.remainder_path_deflate);

	auto target_path_applied = base_test_working / "target.applied";
	auto working_folder      = base_test_working / "working";
	apply_diff(source_path, diff_path, target_path_applied, working_folder);

	VERIFY_FILES_ARE_EQUAL(context.remainder_path, target_path_applied);
}

struct blob_position
{
	int64_t offset;
	int64_t length;
};

class archive_item_definition
{
	public:
	virtual void add_to_diff(diffs::diff *diff) = 0;
	const std::vector<char> &get_data() const { return m_data; }

	virtual void write_inline_asset(std::vector<char> &inline_asset_data) const {}
	virtual void write_source_data(std::vector<char> &source_data) const {}
	virtual bool check_source_data(const std::vector<char> &source_data) const { return true; }

	protected:
	archive_item_definition() = default;
	archive_item_definition(uint64_t length) : m_length(length) {}

	static void populate_from_file(fs::path file_path, std::vector<char> &data, std::vector<char> &hash)
	{
		io_utility::binary_file_reader reader(file_path.string());
		data.resize(reader.size());
		reader.read(0, gsl::span<char>{data.data(), data.size()});
		hash = calculate_hash(data);
	}

	static std::vector<char> calculate_hash(std::vector<char> &data) { return get_hash(data.data(), data.size()); }

	void calculate_hash() { m_hash = calculate_hash(m_data); }

	static void add_buffer_to_buffer(std::vector<char> &target_buffer, const std::vector<char> &source_buffer)
	{
		target_buffer.insert(target_buffer.end(), source_buffer.begin(), source_buffer.end());
	}

	static void add_buffer_to_buffer(
		std::vector<char> &target_buffer, uint64_t offset, const std::vector<char> &source_buffer)
	{
		auto required_size = source_buffer.size() + offset;

		if (target_buffer.size() < required_size)
		{
			target_buffer.resize(required_size);
		}

#ifdef WIN32
		memcpy_s(target_buffer.data() + offset, source_buffer.size(), source_buffer.data(), source_buffer.size());
#else
		memcpy(target_buffer.data() + offset, source_buffer.data(), source_buffer.size());
#endif
	}

	static bool check_buffer_contains_buffer(
		const std::vector<char> &target_buffer, uint64_t offset, const std::vector<char> &source_buffer)
	{
		auto required_size = source_buffer.size() + offset;

		if (target_buffer.size() < required_size)
		{
			return false;
		}

		return 0 == memcmp(target_buffer.data() + offset, source_buffer.data(), source_buffer.size());
	}

	uint64_t m_length{};
	std::vector<char> m_data;
	std::vector<char> m_hash;
};

class copy_source_definition : public archive_item_definition
{
	public:
	copy_source_definition(uint64_t source_offset, size_t size, size_t mod1, size_t mod2) :
		archive_item_definition(size), m_source_offset(source_offset)
	{
		modify_target(m_data, size, mod1, mod2);
		calculate_hash();
	}

	virtual void add_to_diff(diffs::diff *diff)
	{
		auto *chunk = diff->add_chunk(m_length, diffs::hash_type::Sha256, m_hash.data(), m_hash.size());
		auto recipe = chunk->create_recipe(diff, diffs::copy_source_recipe_name);
		recipe->add_number_parameter(m_source_offset);
	}

	virtual void write_source_data(std::vector<char> &source_data) const
	{
		add_buffer_to_buffer(source_data, m_source_offset, m_data);
	}

	virtual bool check_source_data(const std::vector<char> &source_data) const
	{
		return check_buffer_contains_buffer(source_data, m_source_offset, m_data);
	}

	private:
	uint64_t m_source_offset{};
};

class remainder_chunk_definition : public archive_item_definition
{
	public:
	remainder_chunk_definition(uint64_t remainder_offset, size_t size, fs::path remainder_path) :
		archive_item_definition(size)
	{
		m_data.resize(size);

		io_utility::binary_file_reader reader(remainder_path.string());

		reader.read(remainder_offset, gsl::span{m_data.data(), size});

		calculate_hash();
	}

	virtual void add_to_diff(diffs::diff *diff)
	{
		auto *chunk = diff->add_chunk(m_length, diffs::hash_type::Sha256, m_hash.data(), m_hash.size());
		auto recipe = chunk->create_recipe(diff, diffs::remainder_chunk_recipe_name);
	}

	uint64_t source_offset{};
};

class inline_asset_definition : public archive_item_definition
{
	public:
	inline_asset_definition(size_t size, size_t mod1, size_t mod2) : archive_item_definition(size)
	{
		modify_target(m_data, size, mod1, mod2);

		calculate_hash();
	}

	virtual void add_to_diff(diffs::diff *diff)
	{
		auto *chunk = diff->add_chunk(m_length, diffs::hash_type::Sha256, m_hash.data(), m_hash.size());
		auto recipe = chunk->create_recipe(diff, diffs::inline_asset_recipe_name);

		recipe->add_number_parameter(0u);
		recipe->add_number_parameter(static_cast<uint64_t>(m_length));
	}

	virtual void write_inline_asset(std::vector<char> &inline_asset_data) const
	{
		add_buffer_to_buffer(inline_asset_data, m_data);
	}

	private:
};

class nested_diff_definition : public archive_item_definition
{
	public:
	nested_diff_definition(
		fs::path source_path,
		fs::path target_path,
		fs::path diff_path,
		uint64_t source_offset,
		uint64_t inline_asset_offset) :
		m_source_path(source_path),
		m_target_path(target_path), m_diff_path(diff_path), m_source_offset(source_offset),
		m_inline_asset_offset(inline_asset_offset)
	{
		populate_from_file(target_path, m_data, m_hash);
		populate_from_file(source_path, m_source_data, m_source_hash);
		populate_from_file(m_diff_path, m_inline_asset_data, m_inline_asset_hash);

		m_length = m_data.size();
	}

	virtual void add_to_diff(diffs::diff *diff)
	{
		auto *chunk = diff->add_chunk(m_length, diffs::hash_type::Sha256, m_hash.data(), m_hash.size());
		auto recipe = chunk->create_recipe(diff, diffs::nested_diff_recipe_name);

		auto delta_param = recipe->add_archive_item_parameter(
			diffs::archive_item_type::blob,
			0,
			m_inline_asset_data.size(),
			diffs::hash_type::Sha256,
			m_inline_asset_hash.data(),
			m_inline_asset_hash.size());
		auto delta_blob = delta_param->get_archive_item_value();

		auto inline_asset_recipe = delta_blob->create_recipe(diff, diffs::inline_asset_recipe_name);
		inline_asset_recipe->add_number_parameter(0);
		inline_asset_recipe->add_number_parameter(m_inline_asset_data.size());

		auto source_param = recipe->add_archive_item_parameter(
			diffs::archive_item_type::blob,
			0,
			m_source_data.size(),
			diffs::hash_type::Sha256,
			m_source_hash.data(),
			m_source_hash.size());
		auto source_blob = source_param->get_archive_item_value();

		auto copy_source_recipe = source_blob->create_recipe(diff, diffs::copy_source_recipe_name);
		copy_source_recipe->add_number_parameter(m_source_offset);
	}

	virtual void write_inline_asset(std::vector<char> &inline_asset_data) const
	{
		add_buffer_to_buffer(inline_asset_data, m_inline_asset_data);
	}

	virtual void write_source_data(std::vector<char> &source_data) const
	{
		add_buffer_to_buffer(source_data, m_source_offset, m_source_data);
	}

	virtual bool check_source_data(const std::vector<char> &source_data) const
	{
		return check_buffer_contains_buffer(source_data, m_source_offset, m_source_data);
	}

	private:
	fs::path m_source_path;
	fs::path m_target_path;
	fs::path m_diff_path;

	std::vector<char> m_inline_asset_data;
	std::vector<char> m_inline_asset_hash;

	std::vector<char> m_source_data;
	std::vector<char> m_source_hash;

	uint64_t m_source_offset;
	uint64_t m_inline_asset_offset;
};

class make_diff_context
{
	public:
	make_diff_context(fs::path working_directory, fs::path remainder_path, fs::path remainder_path_deflate) :
		m_working_directory(working_directory), m_remainder_path(remainder_path),
		m_remainder_path_deflate(remainder_path_deflate)
	{}

	fs::path get_source_path() const { return m_source_path; }
	fs::path get_target_path() const { return m_target_path; }
	fs::path get_diff_path() const { return m_diff_path; }

	uint64_t get_source_size() const { return m_source_size; }

	void add_item(std::unique_ptr<archive_item_definition> &&item) { m_items.emplace_back(std::move(item)); }

	void create_files()
	{
		std::vector<char> source_data;
		std::vector<char> target_data;
		std::vector<char> inline_asset_data;
		for (const auto& item : m_items)
		{
			item->write_source_data(source_data);
			item->check_source_data(source_data);

			auto& data = item->get_data();
			target_data.insert(target_data.end(), data.begin(), data.end());

			item->write_inline_asset(inline_asset_data);
		}

		for (const auto &item : m_items)
		{
			ASSERT_TRUE(item->check_source_data(source_data));
		}

		fs::create_directories(m_working_directory);

		m_target_path = m_working_directory / "target";
		m_target_hash = get_hash(target_data.data(), target_data.size());
		m_target_size = target_data.size();
		create_file(m_target_path, target_data);

		m_source_path = m_working_directory / "source";
		m_source_hash = get_hash(source_data.data(), source_data.size());
		m_source_size = source_data.size();
		create_file(m_source_path, source_data);

		m_inline_assets_path = m_working_directory / "inline_assets";
		m_inline_assets_hash = get_hash(inline_asset_data.data(), inline_asset_data.size());
		m_inline_assets_size = inline_asset_data.size();
		create_file(m_inline_assets_path, inline_asset_data);

		diffs::diff diff;

		auto uncompressed_remainder_size = fs::file_size(m_remainder_path);
		auto compressed_remainder_size   = fs::file_size(m_remainder_path_deflate);

		diff.set_remainder_sizes(uncompressed_remainder_size, compressed_remainder_size);
		diff.set_target_size(m_target_size);
		diff.set_target_hash(diffs::hash_type::Sha256, m_target_hash.data(), m_target_hash.size());

		diff.set_source_size(m_source_size);
		diff.set_source_hash(diffs::hash_type::Sha256, m_source_hash.data(), m_source_hash.size());

		for (auto &item : m_items)
		{
			item->add_to_diff(&diff);
		}

		m_diff_path = m_working_directory / "diff";
		write_diff(&diff, m_diff_path, m_inline_assets_path, m_remainder_path_deflate);

		m_diff_size = fs::file_size(m_diff_path);
		m_diff_hash = get_hash(m_diff_path);
	}

	private:
	std::vector<std::unique_ptr<archive_item_definition>> m_items;

	fs::path m_working_directory;

	fs::path m_remainder_path;
	fs::path m_remainder_path_deflate;

	fs::path m_inline_assets_path;
	std::vector<char> m_inline_assets_hash;
	uint64_t m_inline_assets_size;

	fs::path m_source_path;
	std::vector<char> m_source_hash;
	uint64_t m_source_size;

	fs::path m_target_path;
	std::vector<char> m_target_hash;
	uint64_t m_target_size;

	fs::path m_diff_path;
	std::vector<char> m_diff_hash;
	uint64_t m_diff_size;
};

void write_data(std::vector<char> &data, io_utility::sequential_writer *writer)
{
	writer->write(std::string_view{data.data(), data.size()});
}

void write_data(blob_position pos, fs::path remainder_path, io_utility::sequential_writer *writer)
{
	std::vector<char> data;
	data.resize(pos.length);

	io_utility::binary_file_reader reader(remainder_path.string());

	reader.read(pos.offset, gsl::span{data.data(), static_cast<size_t>(pos.length)});

	write_data(data, writer);
}

void write_file(fs::path file, io_utility::sequential_writer *writer)
{
	io_utility::binary_file_reader reader(file.string());

	std::vector<char> data;
	data.resize(reader.size());

	reader.read(0, gsl::span{data.data(), data.size()});

	write_data(data, writer);
}

// void process_nested_diff_data(nested_diff_context & context)
//{
//	auto target_writer_raw        = std::make_unique<io_utility::binary_file_writer>(context.target_path);
//	auto nested_target_writer_raw = std::make_unique<io_utility::binary_file_writer>(context.nested_target_path);

//	auto target_writer = std::make_unique<io_utility::wrapped_writer_sequential_writer>(target_writer_raw.get());
//	auto nested_target_writer =
//		std::make_unique<io_utility::wrapped_writer_sequential_writer>(nested_target_writer_raw.get());

//	write_data(context.copy_source_data1, target_writer.get());
//	write_data(context.remainder_1, context.remainder_path, target_writer.get());
//	write_data(context.inline_asset_data1, target_writer.get());
//	write_data(context.remainder_2, context.remainder_path, target_writer.get());
//	write_data(context.copy_source_data2, target_writer.get());

//	write_data(context.nested_remainder_1, context.remainder_path, nested_target_writer.get());
//	write_data(context.nested_copy_source_data1, nested_target_writer.get());
//	write_data(context.nested_inline_asset_data1, nested_target_writer.get());
//	write_data(context.nested_remainder_2, context.remainder_path, nested_target_writer.get());
//	write_data(context.nested_inline_asset_data2, nested_target_writer.get());
//	write_data(context.nested_copy_source_data2, nested_target_writer.get());
//	write_data(context.nested_remainder_3, context.remainder_path, nested_target_writer.get());
//	write_data(context.nested_inline_asset_data3, nested_target_writer.get());
//	write_data(context.nested_remainder_4, context.remainder_path, nested_target_writer.get());

//	nested_target_writer.reset();
//	nested_target_writer_raw.reset();

//	write_file(context.nested_target_path, target_writer.get());

//	write_data(context.remainder_3, context.remainder_path, target_writer.get());
//	write_data(context.copy_source_data3, target_writer.get());
//	write_data(context.inline_asset_data2, target_writer.get());

//	target_writer.reset();
//	target_writer_raw.reset();

//	context.source_hash = get_hash(context.source_path);
//	context.source_size = fs::file_size(context.source_path);

//	context.nested_source_hash = get_hash(context.nested_source_path);
//	context.nested_source_size = fs::file_size(context.nested_source_path);

//	context.target_hash = get_hash(context.target_path);
//	context.target_size = fs::file_size(context.target_path);

//	context.nested_target_hash = get_hash(context.nested_target_path);
//	context.nested_target_size = fs::file_size(context.nested_target_path);
//}

void add_copy_source_to_diff(diffs::diff &diff, std::vector<char> &data, blob_position pos) {}

void add_remainder_to_diff(diffs::diff &diff, blob_position remainder_position, fs::path remainder_path) {}

void add_inline_asset_to_diff(diffs::diff &diff, std::vector<char> &data) {}

void create_nested_diff(make_diff_context &context, fs::path working_dir, fs::path diff_path)
{
	diffs::diff diff;

	auto inline_assets_path = working_dir / "inline_assets";

	// process_nested_diff_data(context);

	// diff.set_target_hash(diffs::hash_type::Sha256, context.target_hash.data(), context.target_hash.size());
	// diff.set_target_size(context.size);

	// add_copy_source_to_diff(diff, context.copy_source_data1, context.copy_source_location1);
	// add_remainder_to_diff(diff, context.remainder_1, context.remainder_path);
	// add_inline_asset_to_diff(diff, context.inline_asset_data1);
	// add_remainder_to_diff(diff, context.remainder_2, context.remainder_path);
	// add_copy_source_to_diff(diff, context.copy_source_data2, context.copy_source_location2);

	// diffs::diff nested_diff;

	// nested_diff.set_target_hash(
	//	diffs::hash_type::Sha256, context.nested_target_hash.data(), context.nested_target_hash.size());
	// diff.set_target_size(context.nested_size);

	// auto nested_chunk = diff.add_chunk(
	//	context.nested_size,
	//	diffs::hash_type::Sha256,
	//	context.nested_target_hash.data(),
	//	context.nested_target_hash.size());
	// auto nested_recipe = nested_chunk->create_recipe(diffs::recipe_type::nested_diff);

	// diff.set_remainder_sizes(fs::file_size(context.remainder_path),
	// fs::file_size(context.remainder_path_deflate));

	// write_diff(&diff, diff_path, inline_assets_path, context.remainder_path_deflate);
}

TEST(recipe_nested_diff, basic)
{
	auto test_name = "Recipe_NestedDiff";

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;

	fs::remove_all(base_test_working);
	fs::create_directories(base_test_working);

	auto nested_diff_working = base_test_working / "nested_diff";
	fs::create_directories(nested_diff_working);

	make_diff_context nested_diff_context(
		nested_diff_working,
		get_data_file(c_source_remainder_uncompressed),
		get_data_file(c_source_remainder_compressed));

	nested_diff_context.add_item(std::make_unique<copy_source_definition>(0, 500, 33, 27));
	nested_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(0, 5000, get_data_file(c_source_remainder_uncompressed)));
	nested_diff_context.add_item(std::make_unique<copy_source_definition>(500, 325, 73, 13));
	nested_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(5000, 3333, get_data_file(c_source_remainder_uncompressed)));
	nested_diff_context.add_item(std::make_unique<inline_asset_definition>(33, 73, 13));
	nested_diff_context.add_item(std::make_unique<copy_source_definition>(1200, 733, 43, 66));
	nested_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(8333, 200000, get_data_file(c_source_remainder_uncompressed)));
	nested_diff_context.add_item(std::make_unique<inline_asset_definition>(7777, 33, 77));
	nested_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(208333, 65000, get_data_file(c_source_remainder_uncompressed)));

	nested_diff_context.create_files();

	auto nested_apply_working_folder = base_test_working / "apply_nested";
	auto target_applied              = base_test_working / "nested_target.applied";

	apply_diff(
		nested_diff_context.get_source_path(),
		nested_diff_context.get_diff_path(),
		target_applied,
		nested_apply_working_folder);

	VERIFY_FILES_ARE_EQUAL(nested_diff_context.get_target_path(), target_applied);

	auto outter_diff_working = base_test_working / "outter_diff";
	fs::create_directories(outter_diff_working);

	make_diff_context outter_diff_context(
		outter_diff_working,
		get_data_file(c_source_remainder_uncompressed),
		get_data_file(c_source_remainder_compressed));

	outter_diff_context.add_item(std::make_unique<copy_source_definition>(0, 500, 33, 27));
	outter_diff_context.add_item(std::make_unique<inline_asset_definition>(33, 13, 31));
	outter_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(0, 500, get_data_file(c_source_remainder_uncompressed)));
	outter_diff_context.add_item(std::make_unique<inline_asset_definition>(1003, 103, 29));

	auto nested_diff_source_path = nested_diff_context.get_source_path();
	auto nested_diff_target_path = nested_diff_context.get_target_path();
	auto nested_diff_diff_path   = nested_diff_context.get_diff_path();

	outter_diff_context.add_item(std::make_unique<nested_diff_definition>(
		nested_diff_source_path, nested_diff_target_path, nested_diff_diff_path, 500, 1003));

	outter_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(500, 1500, get_data_file(c_source_remainder_uncompressed)));
	outter_diff_context.add_item(std::make_unique<inline_asset_definition>(33333, 53, 43));
	outter_diff_context.add_item(
		std::make_unique<copy_source_definition>(nested_diff_context.get_source_size() + 3000, 1500, 73, 7));
	outter_diff_context.add_item(
		std::make_unique<remainder_chunk_definition>(2000, 31500, get_data_file(c_source_remainder_uncompressed)));

	outter_diff_context.create_files();

	auto outter_apply_working_folder = base_test_working / "apply_outter";
	auto outter_target_applied       = base_test_working / "outter_target.applied";

	apply_diff(
		outter_diff_context.get_source_path(),
		outter_diff_context.get_diff_path(),
		outter_target_applied,
		outter_apply_working_folder);

	VERIFY_FILES_ARE_EQUAL(nested_diff_context.get_target_path(), target_applied);
}

struct recipe_zstd_delta_context
{
	std::vector<char> source_data;
	std::vector<char> target_data;

	fs::path source_path;
	fs::path target_path;
	fs::path delta_path;
};

void compress_file(fs::path source, fs::path target, fs::path delta);

TEST(recipe_zstd_delta, basic)
{
	recipe_delta_context context;

	std::vector<char> source_data;
	std::vector<char> target_data;

	source_data.resize(5000);

	modify_target(target_data, 5300, 37, 13);

	auto test_name = "Recipe_ZstdDelta";

	auto base_test_working = fs::temp_directory_path() / "diffs_test" / test_name;

	fs::remove_all(base_test_working);
	fs::create_directories(base_test_working);

	context.source_path = base_test_working / "source";
	context.target_path = base_test_working / "target";
	context.delta_path  = base_test_working / "delta";

	write_data_to_file(context.source_path, std::string_view{source_data.data(), source_data.size()});
	write_data_to_file(context.target_path, std::string_view{target_data.data(), target_data.size()});

	compress_file(context.source_path, context.target_path, context.delta_path);

	ASSERT_TRUE(fs::exists(context.delta_path));

	auto diff_path          = base_test_working / "diff";
	auto remainder_path     = base_test_working / "remainder";
	auto& inline_assets_path = context.delta_path;

	std::fstream remainder_stream(remainder_path, std::ios::binary | std::ios::out);
	remainder_stream.close();

	context.recipe_type_name = diffs::zstd_delta_recipe_name;
	create_diff(
		context.source_path,
		context.target_path,
		add_delta_to_diff,
		&context,
		diff_path,
		inline_assets_path,
		remainder_path,
		remainder_path);

	auto target_path_applied = fs::path(context.target_path.string() + ".applied");
	auto working_folder      = base_test_working / "working";
	apply_diff(context.source_path, diff_path, target_path_applied, working_folder);

	VERIFY_FILES_ARE_EQUAL(context.target_path, target_path_applied);
}

TEST(recipe_concatenation, six_parts)
{
	diffs::recipe_host recipe_host;

	/*
	   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24
	         [     A     ]                [      B      ][C ] [    D        ]
	*/
	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 4}
	// B = {8, 9, 10, 11, 12}
	// C = {13}
	// D = {14, 15, 16, 17}

	/*
	   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22
	   [                A                                                ]
	*/
	const char target_data[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20};
	// A = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 9, 10, 11, 12, 13, 14, 15, 16, 17}

	const uint64_t source_a_offset = 2;
	const uint64_t source_a_length = 5;

	const uint64_t source_b_offset = 12;
	const uint64_t source_b_length = 4;

	const uint64_t source_c_offset = 16;
	const uint64_t source_c_length = 1;

	const uint64_t source_d_offset = 17;
	const uint64_t source_d_length = 4;

	const uint64_t target_a_offset = 0;
	const uint64_t target_a_length = 20;

	/* Combine copies of A, B, C, D from source to make Target A */
	// Target A = Concat(Source A, Source A, Source B, Source C, Source C, Source D)

	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);

	diffs::archive_item target_a_chunk{
		target_a_offset,
		target_a_length,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		target_a_hash.data(),
		target_a_hash.size()};

	auto concat_recipe = target_a_chunk.create_recipe(&recipe_host, diffs::concatenation_recipe_name);

	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_a_offset, source_a_length);
	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_a_offset, source_a_length);
	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_b_offset, source_b_length);
	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_c_offset, source_c_length);
	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_c_offset, source_c_length);
	add_copy_source_param(&recipe_host, concat_recipe, source_data, source_d_offset, source_d_length);

	fs::path source_path;
	fs::path target_path;
	fs::path working_folder;

	make_source_and_target_files(
		"TestApply_TestConcatenation_SixParts",
		std::string_view{source_data, sizeof(source_data)},
		&source_path,
		&target_path,
		&working_folder);

	// use scope to cause the context to flush its output
	{
		diffs::blob_cache blob_cache;

		diffs::diff diff;

		auto base_test_working = fs::temp_directory_path() / "TestApply_TestConcatenation_SixParts";

		fs::remove_all(base_test_working);
		fs::create_directories(base_test_working);

		io_utility::binary_file_readerwriter diff_reader((base_test_working / "empty.diff").string());

		diffs::apply_context::root_context_parameters params;
		params.m_diff           = &diff;
		params.m_diff_reader    = &diff_reader;
		params.m_source_file    = source_path.string();
		params.m_target_file    = target_path.string();
		params.m_blob_cache     = &blob_cache;

		diffs::apply_context context = diffs::apply_context::root_context(params);

		target_a_chunk.apply(context);
	}

	std::ifstream target_istream(target_path, std::ios::binary | std::ios::in);

	char target_a_data[target_a_length];
	target_istream.read(target_a_data, target_a_length);

	ASSERT_EQ(0, memcmp(target_a_data, &target_data[target_a_offset], target_a_length));
}

void compress_file(fs::path source, fs::path target, fs::path delta)
{
	std::string command_line = zstd_compress_file.string();

	//  <uncompressed> <basis> <compressed>
	command_line += " " + target.string();
	command_line += " " + source.string();
	command_line += " " + delta.string();

	printf("Executing: %s\n", command_line.c_str());
	system(command_line.c_str());
}

void compress_file(fs::path original_path, fs::path compressed_path)
{
	auto cmd_line = zstd_compress_file.string();
	cmd_line += " ";
	cmd_line += original_path.string();
	cmd_line += " ";
	cmd_line += compressed_path.string();
	printf("Executing: %s\n", cmd_line.c_str());
	system(cmd_line.c_str());
}

void uncompress_file(fs::path original_path, fs::path uncompressed_path)
{
	auto cmd_line = zstd_compress_file.string();
	cmd_line += " -d ";
	cmd_line += original_path.string();
	cmd_line += " ";
	cmd_line += uncompressed_path.string();
	printf("Executing: %s\n", cmd_line.c_str());
	system(cmd_line.c_str());
}

TEST(zstd_compression_writer, basic)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	auto temp_path = fs::temp_directory_path() / "TestApply_TestZstdCompressionWriter";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = get_data_file(c_source_boot_file_compressed);
	auto uncompressed_path         = temp_path / "boot.ext4";
	uncompress_file(compressed_path, uncompressed_path);
	auto& test_file = uncompressed_path;

	auto test_file_compressed         = temp_path / "boot.ext4.zst";
	auto test_file_compressed_applied = temp_path / "boot.ext4.zst.applied";

	{
		//                std::ofstream compressed_file_stream(test_file_compressed, std::ios::binary |
		//                std::ios::out);
		io_utility::binary_file_writer raw_writer(test_file_compressed.string());
		io_utility::wrapped_writer_sequential_hashed_writer hashed_writer(&raw_writer, &hasher);

		size_t test_file_size = fs::file_size(test_file);
		io_utility::zstd_compression_writer writer(&hashed_writer, 1u, 5u, 3u, test_file_size);

		std::ifstream ifstream(test_file, std::ios::in | std::ios::binary);

		auto remaining = test_file_size;
		char data_block[12 * 1024];
		while (remaining)
		{
			auto to_read = std::min(remaining, sizeof(data_block));
			ifstream.read(data_block, to_read);

			writer.write(std::string_view{data_block, to_read});

			remaining -= to_read;
		}

		raw_writer.flush();
	}

	uncompress_file(test_file_compressed, test_file_compressed_applied);
	VERIFY_FILES_ARE_EQUAL(test_file, test_file_compressed_applied);

	fs::path test_file_compressed2 = test_file_compressed;
	test_file_compressed2 += "2";
	printf("test_file: %s\n", test_file.string().c_str());
	printf("test_file_compressed2: %s\n", test_file_compressed2.string().c_str());
	compress_file(test_file, test_file_compressed2);

	fs::path test_file_compressed_applied2 = test_file_compressed_applied;
	test_file_compressed_applied2 += "2";

	uncompress_file(test_file_compressed2, test_file_compressed_applied2);
	VERIFY_FILES_ARE_EQUAL(test_file, test_file_compressed_applied2);

	VERIFY_FILES_ARE_EQUAL(test_file_compressed, test_file_compressed2);
}

bool FileAndReaderHaveIdenticalContent(fs::path path, io_utility::reader *reader, uint64_t offset, uint64_t length)
{
	auto size = fs::file_size(path);

	std::ifstream file(path, std::ios::in | std::ios::binary);

	file.seekg(offset);

	auto remaining = length;

	std::vector<char> read_buffer1;
	std::vector<char> read_buffer2;

	const size_t read_block_size = static_cast<size_t>(32 * 1024);
	read_buffer1.reserve(read_block_size);
	read_buffer2.reserve(read_block_size);

	std::unique_ptr<io_utility::child_reader> reader_with_offset =
		io_utility::child_reader::with_base_offset(reader, offset);
	std::unique_ptr<io_utility::sequential_reader> sequential_reader =
		std::make_unique<io_utility::wrapped_reader_sequential_reader>(reader_with_offset.get());

	while (remaining)
	{
		size_t to_read = std::min(static_cast<size_t>(read_block_size), static_cast<size_t>(remaining));

		file.read(read_buffer1.data(), to_read);
		sequential_reader->read(gsl::span<char>{read_buffer2.data(), to_read});

		if (0 != memcmp(read_buffer1.data(), read_buffer2.data(), to_read))
		{
			return false;
		}

		remaining -= to_read;
	}

	return true;
}

TEST(binary_file_writer, basic)
{
	const fs::path test_file_path = get_data_file(c_source_boot_file_compressed);
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	std::string test_name = "BinaryFileReader";
	auto reader =
		std::make_unique<io_utility::binary_file_reader>(test_file_path.string());

	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(test_file_path, reader.get(), 3000, 50000));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(test_file_path, reader.get(), 300, 500));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(test_file_path, reader.get(), 0, fs::file_size(test_file_path)));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(test_file_path, reader.get(), 10000, 20));
}

TEST(recipe_copy_source, make_reader)
{
	diffs::recipe_host recipe_host;

	const char source_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

	const char target_data[] = {0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

	uint64_t source_a_offset = 0;
	uint64_t source_a_length = sizeof(source_data);

	uint64_t target_a_offset = 0;
	uint64_t target_a_length = sizeof(target_data);

	// Copy Bytes From Source to Target (Source A -> Target A)
	auto target_a_hash = get_hash(&target_data[target_a_offset], target_a_length);

	diffs::archive_item copy_source_chunk{
		target_a_offset,
		target_a_length,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		target_a_hash.data(),
		target_a_hash.size()};

	auto copy_source_recipe =
		copy_source_chunk.create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(source_a_offset);
	copy_source_recipe->add_number_parameter(source_a_length);

	fs::path source_path;
	fs::path target_path;
	fs::path working_folder;

	make_source_and_target_files(
		"TestApply_TestRegionOfCopySource",
		std::string_view{source_data, sizeof(source_data)},
		&source_path,
		&target_path,
		&working_folder);

	std::stringstream diff_istream(std::ios::binary);

	diffs::blob_cache blob_cache;

	diffs::diff diff;

	auto temp_path = fs::temp_directory_path() / "TestApply_TestRegionOfCopySource";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	io_utility::binary_file_readerwriter diff_reader((temp_path / "empty.diff").string());

	diffs::apply_context::root_context_parameters params;
	params.m_diff           = &diff;
	params.m_diff_reader    = &diff_reader;
	params.m_source_file    = source_path.string();
	params.m_target_file    = target_path.string();
	params.m_blob_cache     = &blob_cache;

	diffs::apply_context context = diffs::apply_context::root_context(params);

	auto copy_source_reader = copy_source_recipe->make_reader(context);

	ASSERT_NE(copy_source_reader.get(), nullptr);

	std::vector<char> data_buffer;
	data_buffer.reserve(sizeof(source_data));

	copy_source_reader->read(0, gsl::span<char>{data_buffer.data(), data_buffer.capacity()});

	ASSERT_TRUE(0 == memcmp(data_buffer.data(), source_data, sizeof(source_data)));

	data_buffer.clear();
	copy_source_reader->read(10, gsl::span<char>{data_buffer.data(), 5});
	ASSERT_TRUE(0 == memcmp(data_buffer.data(), &source_data[10], 5));

	data_buffer.clear();
	copy_source_reader->read(3, gsl::span<char>{data_buffer.data(), 13});
	ASSERT_TRUE(0 == memcmp(data_buffer.data(), &source_data[3], 13));

	data_buffer.clear();
	copy_source_reader->read(15, gsl::span<char>{data_buffer.data(), 7});
	ASSERT_TRUE(0 == memcmp(data_buffer.data(), &source_data[15], 7));
}

TEST(recipe_zstd_compression, of_copy_source)
{
	diffs::recipe_host recipe_host;

	auto temp_path = fs::temp_directory_path() / "Recipe_ZstdCompression_OfCopySource";
	printf("Deleting and recreating %s\n", temp_path.string().c_str());
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = get_data_file(c_source_boot_file_compressed);
	auto uncompressed_path         = temp_path / "boot.ext4";
	uncompress_file(compressed_path, uncompressed_path);
	auto& test_file = uncompressed_path;

	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	fs::path target_path = temp_path / "target";

	auto compressed_hash   = get_hash(compressed_path);
	auto uncompressed_hash = get_hash(uncompressed_path);

	auto compressed_hash_string = data_to_hexstring(compressed_hash.data(), compressed_hash.size());
	printf("%s hash: %s\n", compressed_path.string().c_str(), compressed_hash_string.c_str());

	auto uncompressed_hash_string = data_to_hexstring(uncompressed_hash.data(), uncompressed_hash.size());
	printf("%s hash: %s\n", uncompressed_path.string().c_str(), uncompressed_hash_string.c_str());

	auto uncompressed_file_size = fs::file_size(uncompressed_path);
	std::cout << "uncompressed file size: " << std::to_string(uncompressed_file_size) << std::endl;

	auto compressed_file_size = fs::file_size(compressed_path);
	std::cout << "compressed_file_size file size: " << std::to_string(compressed_file_size) << std::endl;

#if 0
	auto uncompressed_hashes = get_hashes(uncompressed_path);
	printf("uncompressed hashes: \n");
	for (int i = 0; i < uncompressed_hashes.size(); i++)
	{
		auto hash_string = data_to_hexstring(uncompressed_hashes[i].data(), uncompressed_hashes[i].size());
		printf("\t%d: %s\n", i, hash_string.c_str());
	}

	auto compressed_hashes = get_hashes(compressed_path);
	printf("compressed hashes: \n");
	for (int i = 0; i < compressed_hashes.size(); i++)
	{
		auto hash_string = data_to_hexstring(compressed_hashes[i].data(), compressed_hashes[i].size());
		printf("\t%d: %s\n", i, hash_string.c_str());
	}
#endif

	diffs::archive_item copy_source_chunk{
		0,
		uncompressed_file_size,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		uncompressed_hash.data(),
		uncompressed_hash.size()};

	diffs::archive_item compressed_chunk{
		0,
		compressed_file_size,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		compressed_hash.data(),
		compressed_hash.size()};

	auto compress_recipe = compressed_chunk.create_recipe(&recipe_host, diffs::zstd_compression_recipe_name);
	auto param           = compress_recipe->add_archive_item_parameter(
        diffs::archive_item_type::blob,
        0,
        uncompressed_file_size,
        diffs::hash_type::Sha256,
        uncompressed_hash.data(),
        uncompressed_hash.size());

	auto uncompressed_blob  = param->get_archive_item_value();
	auto copy_source_recipe = uncompressed_blob->create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(0u); // offset
	copy_source_recipe->add_number_parameter(uncompressed_file_size);

	compress_recipe->add_number_parameter(1);
	compress_recipe->add_number_parameter(5);
	compress_recipe->add_number_parameter(3);

	fs::path working_folder;

	make_working_directory_path("Recipe_ZstdCompression_OfCopySource", &working_folder);

	fs::remove_all(working_folder);
	fs::create_directories(working_folder);

	diffs::blob_cache blob_cache;

	diffs::diff diff;

	io_utility::binary_file_readerwriter diff_reader((temp_path / "empty.diff").string());

	diffs::apply_context::root_context_parameters params;
	params.m_diff           = &diff;
	params.m_diff_reader    = &diff_reader;
	params.m_source_file    = uncompressed_path.string();
	params.m_target_file    = target_path.string();
	params.m_blob_cache     = &blob_cache;

	{
		diffs::apply_context context = diffs::apply_context::root_context(params);

		compressed_chunk.apply(context);
	}

	VERIFY_FILES_ARE_EQUAL(compressed_path, target_path);
}

TEST(recipe_zstd_decompression, of_copy_source)
{
	diffs::recipe_host recipe_host;

	auto temp_path = fs::temp_directory_path() / "Recipe_ZstdDecompression_OfCopySource";

	const fs::path compressed_path = get_data_file(c_source_boot_file_compressed);

	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	auto uncompressed_path = temp_path / "boot.ext4";

	uncompress_file(compressed_path, uncompressed_path);

	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	fs::path target_path = temp_path / "boot.ext4.zst";

	auto compressed_hash   = get_hash(compressed_path);
	auto uncompressed_hash = get_hash(uncompressed_path);

	auto compressed_file_size = fs::file_size(compressed_path);

	diffs::archive_item copy_source_chunk{
		0,
		compressed_file_size,
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		compressed_hash.data(),
		compressed_hash.size()};

	diffs::archive_item decompressed_chunk{
		0,
		fs::file_size(uncompressed_path),
		diffs::archive_item_type::chunk,
		diffs::hash_type::Sha256,
		uncompressed_hash.data(),
		uncompressed_hash.size()};

	auto decompress_recipe = decompressed_chunk.create_recipe(&recipe_host, diffs::zstd_decompression_recipe_name);
	auto param             = decompress_recipe->add_archive_item_parameter(
        diffs::archive_item_type::blob,
        0,
        compressed_file_size,
        diffs::hash_type::Sha256,
        compressed_hash.data(),
        compressed_hash.size());

	auto compressed_blob    = param->get_archive_item_value();
	auto copy_source_recipe = compressed_blob->create_recipe(&recipe_host, diffs::copy_source_recipe_name);
	copy_source_recipe->add_number_parameter(0u); // offset
	copy_source_recipe->add_number_parameter(compressed_file_size);

	fs::path working_folder;

	make_working_directory_path("Recipe_ZstdDecompression_OfCopySource", &working_folder);

	fs::remove_all(working_folder);
	fs::create_directories(working_folder);

	diffs::blob_cache blob_cache;

	diffs::diff diff;

	io_utility::binary_file_readerwriter diff_reader((temp_path / "empty.diff").string());

	diffs::apply_context::root_context_parameters params;
	params.m_diff           = &diff;
	params.m_diff_reader    = &diff_reader;
	params.m_source_file    = compressed_path.string();
	params.m_target_file    = target_path.string();
	params.m_blob_cache     = &blob_cache;

	diffs::apply_context context = diffs::apply_context::root_context(params);

	auto decompress_reader = decompressed_chunk.make_reader(context);

	ASSERT_NE(decompress_reader.get(), nullptr);

	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, decompress_reader.get(), 300, 500));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, decompress_reader.get(), 3000, 50000));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, decompress_reader.get(), 100000, 20));

	auto decompress_reader2 = decompressed_chunk.make_reader(context);
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(
		uncompressed_path, decompress_reader2.get(), 0, fs::file_size(uncompressed_path)));
}

TEST(zstd_decompression_reader, basic)
{
	const fs::path archive_path = get_data_file(c_source_archive);

	auto temp_path = fs::temp_directory_path() / "zstd_decompression_reader";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	const fs::path compressed_path = get_data_file(c_source_boot_file_compressed);
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	auto uncompressed_path = temp_path / "boot.ext4";
	uncompress_file(compressed_path, uncompressed_path);

	std::string test_name = "TestApply_ZstdDecompressionMakeReader2";

	fs::path(target_path);
	make_target_file_path(test_name, &target_path);

	auto compressed_size = fs::file_size(compressed_path);

	auto archive_reader = std::make_unique<io_utility::binary_file_reader>(archive_path.string());
	auto child_reader   = std::make_unique<io_utility::child_reader>(archive_reader.get(), 4024, compressed_size);
	auto zstd_reader =
		std::make_unique<io_utility::zstd_decompression_reader>(child_reader.get(), fs::file_size(uncompressed_path));

	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, zstd_reader.get(), 300, 500));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, zstd_reader.get(), 3000, 50000));
	ASSERT_TRUE(FileAndReaderHaveIdenticalContent(uncompressed_path, zstd_reader.get(), 100000, 20));

	auto zstd_reader2 =
		std::make_unique<io_utility::zstd_decompression_reader>(child_reader.get(), fs::file_size(uncompressed_path));
	ASSERT_TRUE(
		FileAndReaderHaveIdenticalContent(uncompressed_path, zstd_reader2.get(), 0, fs::file_size(uncompressed_path)));
}

struct blob_location_and_actual_hashes
{
	uint64_t m_offset{};
	diffs::blob_definition m_definition{};
	std::vector<char> actual_hash_whole;
	std::vector<char> actual_hash_chunked;

	std::string actual_hash_string_whole;
	std::string actual_hash_string_chunked;
};

void populate_blob_location_hashes(fs::path archive_path, std::vector<blob_location_and_actual_hashes> &locations)
{
	for (auto &entry : locations)
	{
		auto hash_value = get_hash(archive_path, entry.m_offset, entry.m_definition.m_length);
		entry.m_definition.m_hashes.emplace_back(diffs::hash{diffs::hash_type::Sha256, hash_value});
	}
}

TEST(blob_cache, test)
{
	const fs::path test_file_path = get_data_file(c_source_boot_file_compressed);

	std::vector<blob_location_and_actual_hashes> blob_locations{
		{0, 1000}, {500, 20}, {30000, 2000}, {400, 50}, {9000, 1000}};

	populate_blob_location_hashes(test_file_path, blob_locations);

	diffs::blob_cache cache;

	auto reader =
		std::make_unique<io_utility::binary_file_reader>(test_file_path.string());

	for (auto &entry : blob_locations)
	{
		cache.add_blob_source(reader.get(), entry.m_offset, entry.m_definition);
		cache.add_blob_request(entry.m_definition);
	}

	auto blob_locations_map_from_cache = cache.get_blob_locations_for_reader(reader.get());

	for (auto &entry : blob_locations)
	{
		ASSERT_EQ((size_t)1, blob_locations_map_from_cache.count(entry.m_offset));
		auto from_cache       = blob_locations_map_from_cache.equal_range(entry.m_offset);
		auto count_from_cache = (int)std::distance(from_cache.first, from_cache.second);
		ASSERT_EQ(1, count_from_cache);
		ASSERT_EQ(entry.m_definition.m_length, from_cache.first->second.m_length);
		ASSERT_EQ(
			(int)entry.m_definition.m_hashes[0].m_hash_type, (int)from_cache.first->second.m_hashes[0].m_hash_type);
		ASSERT_EQ(
			entry.m_definition.m_hashes[0].m_hash_data.size(), from_cache.first->second.m_hashes[0].m_hash_data.size());
		auto hash_size = entry.m_definition.m_hashes[0].m_hash_data.size();
		ASSERT_EQ(
			0,
			memcmp(
				entry.m_definition.m_hashes[0].m_hash_data.data(),
				from_cache.first->second.m_hashes[0].m_hash_data.data(),
				hash_size));
	}

	for (auto &entry : blob_locations)
	{
		auto reader = cache.get_blob_reader(entry.m_definition);
		ASSERT_TRUE(nullptr == reader.get());
		ASSERT_TRUE(!cache.is_blob_reader_available(entry.m_definition));
	}

	cache.populate_from_reader(reader.get());

	for (auto &entry : blob_locations)
	{
		auto reader = cache.get_blob_reader(entry.m_definition);
		ASSERT_TRUE(nullptr != reader.get());
		ASSERT_TRUE(cache.is_blob_reader_available(entry.m_definition));

		std::vector<char> data;
		auto length = static_cast<size_t>(entry.m_definition.m_length);
		data.reserve(length);

		reader->read(0, gsl::span<char>{data.data(), length});

		auto hash_value = get_hash(data.data(), length);

		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), hash_value.size());
		auto hash_size = hash_value.size();
		ASSERT_EQ(0, memcmp(entry.m_definition.m_hashes[0].m_hash_data.data(), hash_value.data(), hash_size));

		cache.remove_blob_request(entry.m_definition);
		reader = cache.get_blob_reader(entry.m_definition);
		ASSERT_TRUE(nullptr == reader);
		ASSERT_TRUE(!cache.is_blob_reader_available(entry.m_definition));
	}
}

class sleepy_child_reader : public io_utility::child_reader
{
	public:
	sleepy_child_reader(io_utility::reader *reader) : io_utility::child_reader(reader) {}

	protected:
};

TEST(blob_cache, spin_lock)
{
	const fs::path test_file_path = get_data_file(c_source_boot_file_compressed);

	std::vector<blob_location_and_actual_hashes> blob_locations{
		{0, 1000}, {500, 20}, {30000, 2000}, {400, 50}, {9000, 1000}, {60000, 64 * 1024 * 10}};

	populate_blob_location_hashes(test_file_path, blob_locations);

	diffs::blob_cache cache;

	auto file_reader =
		std::make_unique<io_utility::binary_file_reader>(test_file_path.string());
	sleepy_child_reader raw_reader(file_reader.get());

	for (auto &entry : blob_locations)
	{
		cache.add_blob_source(&raw_reader, entry.m_offset, entry.m_definition);
		cache.add_blob_request(entry.m_definition);
	}

	std::atomic<size_t> matches{};

	std::thread populate_readers_thread(populate_readers, std::ref(cache), std::ref(raw_reader));

	auto process_entry = [&](blob_location_and_actual_hashes &entry)
	{
		while (!cache.is_blob_reader_available(entry.m_definition))
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(200ms);
		}

		auto reader = cache.get_blob_reader(entry.m_definition);
		ASSERT_TRUE(nullptr != reader.get());
		ASSERT_TRUE(cache.is_blob_reader_available(entry.m_definition));

		std::vector<char> data;
		auto length = static_cast<size_t>(entry.m_definition.m_length);
		data.reserve(length);
		reader->read(0, gsl::span<char>{data.data(), length});

		entry.actual_hash_whole        = get_hash(data.data(), length);
		entry.actual_hash_string_whole = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_whole.data(), entry.actual_hash_whole.size()});
		entry.actual_hash_chunked        = get_hash(reader.get(), 0, length);
		entry.actual_hash_string_chunked = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_chunked.data(), entry.actual_hash_chunked.size()});
	};

#ifdef EXECUTION_SUPPORTED
	std::for_each(std::execution::par, blob_locations.begin(), blob_locations.end(), process_entry);
#elif defined(_GLIBCXX_PARALLEL)
	__gnu_parallel::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#else
	std::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#endif

	populate_readers_thread.join();

	for (const auto &entry : blob_locations)
	{
		ASSERT_EQ(0, strcmp(entry.actual_hash_string_chunked.c_str(), entry.actual_hash_string_whole.c_str()));
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_chunked.size());
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_whole.size());
		auto hash_size = entry.actual_hash_chunked.size();
		ASSERT_EQ(
			0, memcmp(entry.m_definition.m_hashes[0].m_hash_data.data(), entry.actual_hash_whole.data(), hash_size));
		ASSERT_EQ(0, memcmp(entry.actual_hash_chunked.data(), entry.actual_hash_whole.data(), hash_size));
	}
}

TEST(blob_cache, wait)
{
	const fs::path test_file_path = get_data_file(c_source_boot_file_compressed);

	std::vector<blob_location_and_actual_hashes> blob_locations{
		{0, 8654848},
		{8655008, 864},
		{8655872, 1286284},
		{9942156, 884},
		{9943040, 188553},
		{10131593, 887},
		{10132694, 810},
		{10133504, 118},
		{10133622, 8074},
		{10141696, 597},
		{10142293, 427},
		{10142823, 343961},
		{10486784, 4194304},
	};

	populate_blob_location_hashes(test_file_path, blob_locations);

	diffs::blob_cache cache;

	auto file_reader =
		std::make_unique<io_utility::binary_file_reader>(test_file_path.string());
	sleepy_child_reader raw_reader(file_reader.get());

	for (auto &entry : blob_locations)
	{
		cache.add_blob_source(&raw_reader, entry.m_offset, entry.m_definition);
		cache.add_blob_request(entry.m_definition);
	}

	std::atomic<size_t> matches{};

	std::thread populate_readers_thread(populate_readers, std::ref(cache), std::ref(raw_reader));

	for (auto &entry : blob_locations)
	{
		if (cache.is_blob_reader_available(entry.m_definition))
		{
			matches++;
		}
	}

	ASSERT_EQ(static_cast<size_t>(0), static_cast<size_t>(matches));

	auto process_entry = [&](blob_location_and_actual_hashes &entry)
	{
		auto reader = cache.wait_for_reader(entry.m_definition);

		ASSERT_TRUE(nullptr != reader.get());
		ASSERT_TRUE(cache.is_blob_reader_available(entry.m_definition));

		std::vector<char> data;
		auto length = static_cast<size_t>(entry.m_definition.m_length);
		data.reserve(length);
		reader->read(0, gsl::span<char>{data.data(), length});

		entry.actual_hash_whole        = get_hash(data.data(), length);
		entry.actual_hash_string_whole = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_whole.data(), entry.actual_hash_whole.size()});
		entry.actual_hash_chunked        = get_hash(reader.get(), 0, length);
		entry.actual_hash_string_chunked = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_chunked.data(), entry.actual_hash_chunked.size()});
	};

#ifdef EXECUTION_SUPPORTED
	std::for_each(std::execution::par, blob_locations.begin(), blob_locations.end(), process_entry);
#elif defined(_GLIBCXX_PARALLEL)
	__gnu_parallel::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#else
	std::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#endif

	populate_readers_thread.join();

	for (const auto &entry : blob_locations)
	{
		ASSERT_EQ(0, strcmp(entry.actual_hash_string_chunked.c_str(), entry.actual_hash_string_whole.c_str()));
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_chunked.size());
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_whole.size());
		auto hash_size = entry.actual_hash_chunked.size();
		ASSERT_EQ(
			0, memcmp(entry.m_definition.m_hashes[0].m_hash_data.data(), entry.actual_hash_whole.data(), hash_size));
		ASSERT_EQ(0, memcmp(entry.actual_hash_chunked.data(), entry.actual_hash_whole.data(), hash_size));
	}
}

TEST(blob_cache, decompressed_data_wait)
{
	const fs::path compressed_path = get_data_file(c_source_boot_file_compressed);

	auto temp_path = fs::temp_directory_path() / "blob_cache.decompressed_data_wait";
	fs::remove_all(temp_path);
	fs::create_directories(temp_path);

	auto uncompressed_path = temp_path / "boot.ext4";
	uncompress_file(compressed_path, uncompressed_path);
	auto& test_file = uncompressed_path;

	std::vector<blob_location_and_actual_hashes> blob_locations{
		{0, 8654848},
		{8655008, 864},
		{8655872, 1286284},
		{9942156, 884},
		{9943040, 188553},
		{10131593, 887},
		{10132694, 810},
		{10133504, 118},
		{10133622, 8074},
		{10141696, 597},
		{10142293, 427},
		{10142823, 343961},
		{10486784, 4194304},
	};

	populate_blob_location_hashes(uncompressed_path, blob_locations);

	diffs::blob_cache cache;

	io_utility::binary_file_reader file_reader(compressed_path.string());
	io_utility::zstd_decompression_reader decompression_reader(&file_reader, fs::file_size(uncompressed_path));
	sleepy_child_reader raw_reader(&decompression_reader);

	for (auto &entry : blob_locations)
	{
		cache.add_blob_source(&raw_reader, entry.m_offset, entry.m_definition);
		cache.add_blob_request(entry.m_definition);
	}

	std::atomic<size_t> matches{};

	std::thread populate_readers_thread(populate_readers, std::ref(cache), std::ref(raw_reader));

	for (auto &entry : blob_locations)
	{
		if (cache.is_blob_reader_available(entry.m_definition))
		{
			matches++;
		}
	}

	ASSERT_EQ(static_cast<size_t>(0), static_cast<size_t>(matches));

	auto process_entry = [&](blob_location_and_actual_hashes &entry)
	{
		auto reader = cache.wait_for_reader(entry.m_definition);

		ASSERT_TRUE(nullptr != reader.get());
		ASSERT_TRUE(cache.is_blob_reader_available(entry.m_definition));

		std::vector<char> data;
		auto length = static_cast<size_t>(entry.m_definition.m_length);
		data.reserve(length);
		reader->read(0, gsl::span<char>{data.data(), length});

		entry.actual_hash_whole        = get_hash(data.data(), length);
		entry.actual_hash_string_whole = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_whole.data(), entry.actual_hash_whole.size()});
		entry.actual_hash_chunked        = get_hash(reader.get(), 0, length);
		entry.actual_hash_string_chunked = diffs::hash::get_hex_string(
			std::string_view{entry.actual_hash_chunked.data(), entry.actual_hash_chunked.size()});
	};

#ifdef EXECUTION_SUPPORTED
	std::for_each(std::execution::par, blob_locations.begin(), blob_locations.end(), process_entry);
#elif defined(_GLIBCXX_PARALLEL)
	__gnu_parallel::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#else
	std::for_each(blob_locations.begin(), blob_locations.end(), process_entry);
#endif

	populate_readers_thread.join();

	for (const auto &entry : blob_locations)
	{
		ASSERT_EQ(0, strcmp(entry.actual_hash_string_chunked.c_str(), entry.actual_hash_string_whole.c_str()));
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_chunked.size());
		ASSERT_EQ(entry.m_definition.m_hashes[0].m_hash_data.size(), entry.actual_hash_whole.size());
		auto hash_size = entry.actual_hash_chunked.size();
		ASSERT_EQ(
			0, memcmp(entry.m_definition.m_hashes[0].m_hash_data.data(), entry.actual_hash_whole.data(), hash_size));
		ASSERT_EQ(0, memcmp(entry.actual_hash_chunked.data(), entry.actual_hash_whole.data(), hash_size));
	}
}

TEST(apply_context, basic)
{
	char inline_assets_data[] = {
		'I', 'N', 'L', 'I', 'N', 'E', ' ', 'A', 'S', 'S', 'E', 'T', 'S', ' ', 'D', 'A', 'T', 'A'};
	char remainder_data[] = {'R', 'E', 'M', 'A', 'I', 'D', 'E', 'R'};

	auto inline_assets_data_vector = std::make_shared<std::vector<char>>();
	auto remainder_data_vector     = std::make_shared<std::vector<char>>();

	inline_assets_data_vector->reserve(sizeof(inline_assets_data));
	memcpy(inline_assets_data_vector->data(), inline_assets_data, sizeof(inline_assets_data));

	remainder_data_vector->reserve(sizeof(remainder_data));
	memcpy(remainder_data_vector->data(), remainder_data, sizeof(remainder_data));

	io_utility::stored_blob_reader inline_assets_data_reader(inline_assets_data_vector, sizeof(inline_assets_data));
	io_utility::stored_blob_reader remainder_reader(remainder_data_vector, sizeof(remainder_data));
}
