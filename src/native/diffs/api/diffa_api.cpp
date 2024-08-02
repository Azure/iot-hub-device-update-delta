/**
 * @file diffa_api.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <memory>

#include <errors/user_exception.h>

#include "diffa_api.h"
#include "apply_session.h"

#include <diffs/core/item_definition.h>
#include <aduapi_type_conversion.h>

ADUAPI_LINKAGESPEC diffa_handle CDECL diffa_open_session()
{
	auto session = std::make_unique<archive_diff::diffs::api::apply_session>();
	return reinterpret_cast<diffa_handle>(session.release());
}

ADUAPI_LINKAGESPEC void CDECL diffa_close_session(diffa_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	delete session;
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_add_archive(diffa_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->add_archive(path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_request_item(diffa_handle handle, const diffc_item_definition *item)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	auto converted_item = diffc_item_definition_to_core_item_definition(*item);

	return session->request_item(converted_item);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_add_file_to_pantry(diffa_handle handle, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->add_file_to_pantry(path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_clear_requested_items(diffa_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->clear_requested_items();
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_process_requested_items(diffa_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->process_requested_items() ? 1 : 0;
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_resume_slicing(diffa_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->resume_slicing();
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_cancel_slicing(diffa_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	return session->cancel_slicing();
}

ADUAPI_LINKAGESPEC uint32_t CDECL
diffa_extract_item_to_path(diffa_handle handle, const diffc_item_definition *item, const char *path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	auto converted_item = diffc_item_definition_to_core_item_definition(*item);

	return session->extract_item_to_path(converted_item, path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_get_error_code(diffa_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->get_error_code(index);
}

ADUAPI_LINKAGESPEC const char *CDECL diffa_get_error_text(diffa_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->get_error_text(index);
}