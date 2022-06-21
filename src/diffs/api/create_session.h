/**
 * @file create_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "adudiffcreate.h"

#ifndef CDECL
	#ifdef WIN32
		#define CDECL __cdecl
	#else
		#define CDECL
	#endif
#endif

namespace fs = std::filesystem;

namespace diffs
{
class diff;
namespace api
{
class create_session
{
	public:
	CDECL create_session();
	CDECL ~create_session();

	void CDECL set_remainder_sizes(uint64_t uncompressed, uint64_t compressed);
	void CDECL set_target_size_and_hash(
		uint64_t size, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length);
	void CDECL set_source_size_and_hash(
		uint64_t size, adu_create_hash_type hash_type, const char *hash_value, size_t hash_value_length);

	adu_create_archive_item CDECL add_chunk(
		uint64_t offset,
		uint64_t length,
		adu_create_hash_type hash_type,
		const char *hash_value,
		size_t hash_value_length);
	adu_create_recipe CDECL create_recipe(adu_create_archive_item item, adu_create_recipe_type type);
	int CDECL add_recipe_parameter_number(adu_create_recipe recipe_handle, uint64_t number);
	adu_create_archive_item CDECL add_recipe_parameter_archive_item(
		adu_create_recipe recipe_handle,
		adu_create_archive_item_type type,
		uint64_t offset,
		uint64_t length,
		adu_create_hash_type hash_type,
		const char *hash_value,
		size_t hash_value_length);

	void CDECL set_inline_asset_path(const char *path) { inline_asset_path = path; }
	void CDECL set_remainder_path(const char *path) { remainder_path = path; }
	void CDECL write(const char *path);

	private:
	diffs::diff *diff{};

	fs::path inline_asset_path;
	fs::path remainder_path;
};
} // namespace api
} // namespace diffs