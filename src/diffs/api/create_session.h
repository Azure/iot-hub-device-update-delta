/**
 * @file create_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <vector>

#include "adudiffcreate.h"

#ifndef CDECL
	#ifdef WIN32
		#define CDECL __cdecl
	#else
		#define CDECL
	#endif
#endif

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
	adu_create_recipe CDECL create_recipe(adu_create_archive_item item, const char* recipe_type_name);
	int CDECL add_recipe_parameter_number(adu_create_recipe recipe_handle, uint64_t number);
	adu_create_archive_item CDECL add_recipe_parameter_archive_item(
		adu_create_recipe recipe_handle,
		adu_create_archive_item_type type,
		uint64_t offset,
		uint64_t length,
		adu_create_hash_type hash_type,
		const char *hash_value,
		size_t hash_value_length);

	void CDECL set_inline_asset_path(const char *path) { m_inline_asset_path = path; }
	void CDECL set_remainder_path(const char *path) { m_remainder_path = path; }
	void CDECL write(const char *path);

	private:
	diffs::diff *m_diff{};

	std::string m_inline_asset_path;
	std::string m_remainder_path;
};
} // namespace api
} // namespace diffs