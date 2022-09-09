/**
 * @file create_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "create_session.h"

#include "diff.h"
#include "hash.h"
#include "archive_item.h"

#include "user_exception.h"

#include "binary_file_reader.h"
#include "binary_file_readerwriter.h"
#include "binary_file_writer.h"

#include "adudiffcreate.h"

CDECL diffs::api::create_session::create_session() { m_diff = new diffs::diff(); }

CDECL diffs::api::create_session::~create_session() { delete m_diff; }

void CDECL diffs::api::create_session::set_remainder_sizes(uint64_t uncompressed, uint64_t compressed)
{
	m_diff->set_remainder_sizes(uncompressed, compressed);
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
	m_diff->set_target_size(size);

	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	m_diff->set_target_hash(real_hash_type, hash_value, hash_value_length);
}

void CDECL diffs::api::create_session::set_source_size_and_hash(
	uint64_t size, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length)
{
	m_diff->set_source_size(size);

	auto real_hash_type = adu_create_hash_type_to_hash_type(hash_type);

	if (size)
	{
		m_diff->set_source_hash(real_hash_type, hash_value, hash_value_length);
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
		m_diff->add_chunk(offset, length, real_hash_type, hash_value, hash_value_length));
}

adu_create_recipe CDECL
diffs::api::create_session::create_recipe(adu_create_archive_item item, const char* recipe_type_name)
{
	auto archive_item = reinterpret_cast<diffs::archive_item *>(item);

	return static_cast<adu_create_recipe>(archive_item->create_recipe(m_diff, recipe_type_name));
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
	io_utility::binary_file_reader inline_asset_reader(m_inline_asset_path);
	io_utility::binary_file_reader remainder_reader(m_remainder_path);

	diff_writer_context context{&diff_writer, &inline_asset_reader, &remainder_reader};
	m_diff->write(context);
}