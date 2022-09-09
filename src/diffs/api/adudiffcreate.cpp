/**
 * @file adudiffcreate.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <memory>

#include "adudiffcreate.h"
#include "create_session.h"

#include "user_exception.h"

ADUAPI_LINKAGESPEC adu_create_handle CDECL adu_diff_create_create_session()
{
	auto session = std::make_unique<diffs::api::create_session>();
	return reinterpret_cast<adu_create_handle>(session.release());
}

ADUAPI_LINKAGESPEC void CDECL adu_diff_create_close_session(adu_create_handle handle)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	delete session;
}

ADUAPI_LINKAGESPEC void CDECL
adu_diff_create_set_remainder_sizes(adu_create_handle handle, uint64_t uncompressed, uint64_t compressed)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);
	session->set_remainder_sizes(uncompressed, compressed);
}

ADUAPI_LINKAGESPEC void CDECL adu_diff_create_set_target_size_and_hash(
	adu_create_handle handle,
	uint64_t size,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);
	session->set_target_size_and_hash(size, hash_type, hash_value, hash_value_length);
}
ADUAPI_LINKAGESPEC void CDECL adu_diff_create_set_source_size_and_hash(
	adu_create_handle handle,
	uint64_t size,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);
	session->set_source_size_and_hash(size, hash_type, hash_value, hash_value_length);
}

ADUAPI_LINKAGESPEC adu_create_archive_item CDECL adu_diff_create_add_chunk(
	adu_create_handle handle,
	uint64_t offset,
	uint64_t length,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	return session->add_chunk(offset, length, hash_type, hash_value, hash_value_length);
}

ADUAPI_LINKAGESPEC adu_create_recipe CDECL
adu_diff_create_add_recipe(adu_create_handle handle, adu_create_archive_item item, const char* recipe_type_name)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	return session->create_recipe(item, recipe_type_name);
}

ADUAPI_LINKAGESPEC adu_create_archive_item CDECL adu_diff_create_add_recipe_parameter_archive_item(
	adu_create_handle handle,
	adu_create_recipe recipe,
	adu_create_archive_item_type type,
	uint64_t offset,
	uint64_t length,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	return session->add_recipe_parameter_archive_item(
		recipe, type, offset, length, hash_type, hash_value, hash_value_length);
}

ADUAPI_LINKAGESPEC int CDECL
adu_diff_create_add_recipe_parameter_number(adu_create_handle handle, adu_create_recipe recipe, uint64_t number)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	return session->add_recipe_parameter_number(recipe, number);
}

ADUAPI_LINKAGESPEC int CDECL adu_diff_create_set_inline_asset_path(adu_create_handle handle, const char *path)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);
	session->set_inline_asset_path(path);

	return 0;
}

ADUAPI_LINKAGESPEC int CDECL adu_diff_create_set_remainder_path(adu_create_handle handle, const char *path)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);
	session->set_remainder_path(path);

	return 0;
}

ADUAPI_LINKAGESPEC int CDECL adu_diff_create_write(adu_create_handle handle, const char *path)
{
	auto session = reinterpret_cast<diffs::api::create_session *>(handle);

	try
	{
		session->write(path);
	}
	catch (error_utility::user_exception &e)
	{
		return static_cast<int>(e.get_error());
	}
	catch (std::exception &)
	{
		return static_cast<int>(error_utility::error_code::standard_library_exception);
	}

	return 0;
}
