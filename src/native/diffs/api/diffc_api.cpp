/**
 * @file diffc_api.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <memory>

#include <errors/user_exception.h>

#include "diffc_api.h"
#include "create_session.h"

ADUAPI_LINKAGESPEC diffc_handle CDECL diffc_open_session()
{
	auto session = std::make_unique<archive_diff::diffs::api::create_session>();
	return reinterpret_cast<diffc_handle>(session.release());
}

ADUAPI_LINKAGESPEC void CDECL diffc_close_session(diffc_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	delete session;
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_target(diffc_handle handle, const diffc_item_definition *item)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->set_target(*item);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_source(diffc_handle handle, const diffc_item_definition *item)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->set_source(*item);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_add_recipe(
	diffc_handle handle,
	const char *recipe_name,
	const diffc_item_definition *result_item,
	uint64_t *number_ingredients,
	size_t number_ingredients_count,
	const diffc_item_definition **item_ingredients,
	size_t item_ingredients_count)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->add_recipe(
		recipe_name,
		result_item,
		number_ingredients,
		number_ingredients_count,
		item_ingredients,
		item_ingredients_count);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_inline_assets(diffc_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->set_inline_assets(path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_remainder(diffc_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->set_remainder(path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_add_nested_archive(diffc_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->add_nested_archive(path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL
diffc_add_payload(diffc_handle handle, const char *name, const diffc_item_definition &item)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->add_payload(name, item);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_write_diff(diffc_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->write_diff(path);
}

ADUAPI_LINKAGESPEC diffa_handle CDECL diffc_new_diffa_session(diffc_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);

	auto apply_session = session->new_apply_session();
	return static_cast<diffa_handle>(apply_session);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_get_error_code(diffc_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->get_error_code(index);
}

ADUAPI_LINKAGESPEC const char *CDECL diffc_get_error_text(diffc_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::create_session *>(handle);
	return session->get_error_text(index);
}