/**
 * @file create_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <filesystem>

#include "create_session.h"

#include "diff.h"
#include "hash.h"
#include "archive_item.h"

#include "user_exception.h"

#include "binary_file_reader.h"
#include "binary_file_readerwriter.h"
#include "binary_file_writer.h"

#include "adudiffcreate.h"

CDECL diffs::api::create_session::create_session() { diff = new diffs::diff(); }

CDECL diffs::api::create_session::~create_session() { delete diff; }

void CDECL diffs::api::create_session::set_remainder_sizes(uint64_t uncompressed, uint64_t compressed)
{
	diff->set_remainder_sizes(uncompressed, compressed);
}

diffs::hash_type CDECL adu_create_hash_type_to_hash_type(adu_create_hash_type type)
{
	switch (type)
	{
	case adu_create_hash_type::adu_create_hash_type_md5:
		return diffs::hash_type::Md5;
	case adu_create_hash_type::adu_create_hash_sha256:
		return diffs::hash_type::Sha256;
	}
	std::string msg = "Unexpected hash type: " + std::to_string(static_cast<int>(type));
	throw error_utility::user_exception(error_utility::error_code::api_unexpected_hash_type, msg);
}

void CDECL diffs::api::create_session::set_target_size_and_hash(
	uint64_t size, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length)
{
	diff->set_target_size(size);

	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	diff->set_target_hash(real_hash_type, hash_value, hash_value_length);
}

void CDECL diffs::api::create_session::set_source_size_and_hash(
	uint64_t size, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length)
{
	diff->set_source_size(size);

	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	if (size)
	{
		diff->set_source_hash(real_hash_type, hash_value, hash_value_length);
	}
}

diffs::archive_item_type CDECL adu_create_archive_item_type_to_archive_item_type(adu_create_archive_item_type type)
{
	switch (type)
	{
	case adu_create_archive_item_type::adu_create_archive_item_type_blob:
		return diffs::archive_item_type::blob;
	case adu_create_archive_item_type::adu_create_archive_item_type_chunk:
		return diffs::archive_item_type::chunk;
	case adu_create_archive_item_type::adu_create_archive_item_type_payload:
		return diffs::archive_item_type::payload;
	}

	std::string msg = "Unexpected archive item type: " + std::to_string(static_cast<int>(type));
	throw error_utility::user_exception(error_utility::error_code::api_unexpected_archive_item_type, msg);
}

adu_create_archive_item CDECL diffs::api::create_session::add_chunk(
	uint64_t offset, uint64_t length, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length)
{
	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	return static_cast<adu_create_archive_item>(
		diff->add_chunk(offset, length, real_hash_type, hash_value, hash_value_length));
}

diffs::recipe_type CDECL adu_create_recipe_type_to_recipe_type(adu_create_recipe_type type)
{
	switch (type)
	{
	case adu_create_recipe_type::adu_create_recipe_type_copy:
		return diffs::recipe_type::copy;
	case adu_create_recipe_type::adu_create_recipe_type_region:
		return diffs::recipe_type::region;
	case adu_create_recipe_type::adu_create_recipe_type_concatenation:
		return diffs::recipe_type::concatenation;
	case adu_create_recipe_type::adu_create_recipe_type_apply_bsdiff:
		return diffs::recipe_type::bsdiff_delta;
	case adu_create_recipe_type::adu_create_recipe_type_apply_nested:
		return diffs::recipe_type::nested_diff;
	case adu_create_recipe_type::adu_create_recipe_type_remainder_chunk:
		return diffs::recipe_type::remainder_chunk;
	case adu_create_recipe_type::adu_create_recipe_type_inline_asset:
		return diffs::recipe_type::inline_asset;
	case adu_create_recipe_type::adu_create_recipe_type_copy_source:
		return diffs::recipe_type::copy_source;
	case adu_create_recipe_type::adu_create_recipe_type_apply_zstd_delta:
		return diffs::recipe_type::zstd_delta;
	case adu_create_recipe_type::adu_create_recipe_type_inline_asset_copy:
		return diffs::recipe_type::inline_asset_copy;
	case adu_create_recipe_type::adu_create_recipe_type_zstd_compression:
		return diffs::recipe_type::zstd_compression;
	case adu_create_recipe_type::adu_create_recipe_type_zstd_decompression:
		return diffs::recipe_type::zstd_decompression;
	case adu_create_recipe_type::adu_create_recipe_type_all_zero:
		return diffs::recipe_type::all_zero;
	case adu_create_recipe_type::adu_create_recipe_type_gz_decompression:
		return diffs::recipe_type::gz_decompression;
	}

	std::string msg = "Unexpected recipe type: " + std::to_string(static_cast<int>(type));
	throw error_utility::user_exception(error_utility::error_code::api_unexpected_recipe_type, msg);
}

adu_create_recipe CDECL
diffs::api::create_session::create_recipe(adu_create_archive_item item, adu_create_recipe_type type)
{
	auto archive_item = reinterpret_cast<diffs::archive_item *>(item);

	auto real_type = adu_create_recipe_type_to_recipe_type(type);

	return static_cast<adu_create_recipe>(archive_item->create_recipe(real_type));
}

adu_create_archive_item CDECL diffs::api::create_session::add_recipe_parameter_archive_item(
	adu_create_recipe recipe_handle,
	adu_create_archive_item_type type,
	uint64_t offset,
	uint64_t length,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	auto recipe = reinterpret_cast<diffs::recipe *>(recipe_handle);

	auto real_type      = adu_create_archive_item_type_to_archive_item_type(type);
	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	auto new_param =
		recipe->add_archive_item_parameter(real_type, offset, length, real_hash_type, hash_value, hash_value_length);

	return static_cast<adu_create_archive_item>(new_param->get_archive_item_value());
}

int CDECL diffs::api::create_session::add_recipe_parameter_number(adu_create_recipe recipe_handle, uint64_t number)
{
	auto recipe = reinterpret_cast<diffs::recipe *>(recipe_handle);

	recipe->add_number_parameter(number);

	return 0;
}

void CDECL diffs::api::create_session::write(const char *path)
{
	io_utility::binary_file_writer diff_writer(path);
	io_utility::binary_file_reader inline_asset_reader(inline_asset_path);
	io_utility::binary_file_reader remainder_reader(remainder_path);

	diff_writer_context context{&diff_writer, &inline_asset_reader, &remainder_reader};
	diff->write(context);
}