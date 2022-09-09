/**
 * @file adudiffcreate.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "adudiffapi.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef void *adu_create_handle;
typedef void *adu_create_archive_item;
typedef void *adu_create_recipe;

#ifdef __cplusplus
enum class adu_create_archive_item_type
#else
typedef enum
#endif
{
	adu_create_archive_item_type_blob    = 0,
	adu_create_archive_item_type_chunk   = 1,
	adu_create_archive_item_type_payload = 2,
#ifdef __cplusplus
};
#else
} adu_create_archive_item_type;
#endif

#ifdef __cplusplus
enum class adu_create_hash_type
#else
typedef enum
#endif
{
	adu_create_hash_type_md5 = 0,
	adu_create_hash_sha256   = 1,
#ifdef __cplusplus
};
#else
} adu_create_hash_type;
#endif

ADUAPI_LINKAGESPEC adu_create_handle CDECL adu_diff_create_create_session();
ADUAPI_LINKAGESPEC void CDECL adu_diff_create_close_session(adu_create_handle handle);
ADUAPI_LINKAGESPEC void CDECL
adu_diff_create_set_remainder_sizes(adu_create_handle handle, uint64_t uncompressed, uint64_t compressed);
ADUAPI_LINKAGESPEC void CDECL adu_diff_create_set_target_size_and_hash(
	adu_create_handle handle,
	uint64_t size,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length);
ADUAPI_LINKAGESPEC void CDECL adu_diff_create_set_source_size_and_hash(
	adu_create_handle handle,
	uint64_t size,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length);
ADUAPI_LINKAGESPEC adu_create_archive_item CDECL adu_diff_create_add_chunk(
	adu_create_handle handle,
	uint64_t offset,
	uint64_t length,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length);
ADUAPI_LINKAGESPEC adu_create_recipe CDECL
adu_diff_create_add_recipe(adu_create_handle handle, adu_create_archive_item item_handle, const char* recipe_type_name);
ADUAPI_LINKAGESPEC adu_create_archive_item CDECL adu_diff_create_add_recipe_parameter_archive_item(
	adu_create_handle handle,
	adu_create_recipe recipe_handle,
	adu_create_archive_item_type type,
	uint64_t offset,
	uint64_t length,
	adu_create_hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length);
ADUAPI_LINKAGESPEC int CDECL
adu_diff_create_add_recipe_parameter_number(adu_create_handle handle, adu_create_recipe recipe_handle, uint64_t number);
ADUAPI_LINKAGESPEC int CDECL adu_diff_create_set_inline_asset_path(adu_create_handle handle, const char *path);
ADUAPI_LINKAGESPEC int CDECL adu_diff_create_set_remainder_path(adu_create_handle handle, const char *path);
ADUAPI_LINKAGESPEC int CDECL adu_diff_create_write(adu_create_handle handle, const char *path);

#ifdef __cplusplus
}
#endif