/**
 * @file diffa_api.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <io/user/user_readerwriter_pfn.h>

#include "adudiffapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *diffa_handle;

#include "aduapi_types.h"

ADUAPI_LINKAGESPEC diffa_handle CDECL diffa_open_session();
ADUAPI_LINKAGESPEC void CDECL diffa_close_session(diffa_handle handle);

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_add_archive(diffa_handle handle, const char *path);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_request_item(diffa_handle handle, const diffc_item_definition *item);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_add_file_to_pantry(diffa_handle handle, const char *path);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_clear_requested_items(diffa_handle handle);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_process_requested_items(diffa_handle handle);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_process_requested_items_ex(
	diffa_handle handle,
	bool select_recipes_only,
	const diffc_item_definition **mocked_items,
	size_t mocked_item_count);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_save_selected_recipes(diffa_handle handle, const char *path);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_resume_slicing(diffa_handle handle);
ADUAPI_LINKAGESPEC uint32_t CDECL diffa_cancel_slicing(diffa_handle handle);
ADUAPI_LINKAGESPEC uint32_t CDECL
diffa_extract_item_to_path(diffa_handle handle, const diffc_item_definition *item, const char *path);

ADUAPI_LINKAGESPEC uint32_t CDECL diffa_get_error_code(diffa_handle handle, uint32_t index);
ADUAPI_LINKAGESPEC const char *CDECL diffa_get_error_text(diffa_handle handle, uint32_t index);
#ifdef __cplusplus
}
#endif