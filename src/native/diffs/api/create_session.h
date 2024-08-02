/**
 * @file create_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <vector>
#include <memory>

#include <diffs/serialization/standard/deserializer.h>

#include "aduapi_types.h"
#include "session_base.h"

#ifndef CDECL
	#ifdef WIN32
		#define CDECL __cdecl
	#else
		#define CDECL
	#endif
#endif

namespace archive_diff::diffs
{
class archive;
namespace api
{
class apply_session;

class create_session : public session_base
{
	public:
	create_session()  = default;
	~create_session() = default;

	uint32_t set_target(const diffc_item_definition &item);
	uint32_t set_source(const diffc_item_definition &item);

	uint32_t set_inline_assets(const char *path);
	uint32_t set_remainder(const char *path);
	uint32_t add_nested_archive(const char *path);

	uint32_t add_payload(const char *payload_name, const diffc_item_definition &item);

	uint32_t add_recipe(
		const char *recipe_name,
		const diffc_item_definition *result_item,
		uint64_t *number_ingredients,
		size_t number_ingredients_count,
		const diffc_item_definition **item_ingredients,
		size_t item_ingredient_count);

	uint32_t write_diff(const char *path);

	apply_session *new_apply_session() const;

	private:
	std::mutex m_archive_mutex;
	diffs::serialization::standard::deserializer m_deserializer;
};
} // namespace api
} // namespace archive_diff::diffs