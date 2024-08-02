/**
 * @file diffc_api.h
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

typedef void *diffc_handle;

#include "aduapi_types.h"

#include "diffa_api.h"

ADUAPI_LINKAGESPEC diffc_handle CDECL diffc_open_session();
ADUAPI_LINKAGESPEC void CDECL diffc_close_session(diffc_handle handle);

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_target(diffc_handle handle, const diffc_item_definition *item);
ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_source(diffc_handle handle, const diffc_item_definition *item);

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_add_recipe(
	diffc_handle handle,
	const char *recipe_name,
	const diffc_item_definition *result_item,
	uint64_t *number_ingredients,
	size_t number_ingredients_count,
	const diffc_item_definition **item_ingredients,
	size_t item_ingredients_count);

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_inline_assets(diffc_handle handle, const char *path);
ADUAPI_LINKAGESPEC uint32_t CDECL diffc_set_remainder(diffc_handle handle, const char *path);
ADUAPI_LINKAGESPEC uint32_t CDECL diffc_add_nested_archive(diffc_handle, const char *path);

ADUAPI_LINKAGESPEC uint32_t CDECL
diffc_add_payload(diffc_handle handle, const char *name, const diffc_item_definition &item);

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_write_diff(diffc_handle handle, const char *path);

ADUAPI_LINKAGESPEC diffa_handle CDECL diffc_new_diffa_session(diffc_handle handle);

ADUAPI_LINKAGESPEC uint32_t CDECL diffc_get_error_code(diffc_handle handle, uint32_t index);
ADUAPI_LINKAGESPEC const char *CDECL diffc_get_error_text(diffc_handle handle, uint32_t index);
#ifdef __cplusplus
}
#endif