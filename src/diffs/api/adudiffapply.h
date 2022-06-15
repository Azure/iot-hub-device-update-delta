/**
 * @file adudiffapply.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "adudiffapi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *adu_apply_handle;

ADUAPI_LINKAGESPEC adu_apply_handle CDECL adu_diff_apply_create_session();
ADUAPI_LINKAGESPEC void CDECL adu_diff_apply_close_session(adu_apply_handle handle);
ADUAPI_LINKAGESPEC int CDECL adu_diff_apply_set_log_path(adu_apply_handle handle, const char *log_path);
ADUAPI_LINKAGESPEC int CDECL
adu_diff_apply(adu_apply_handle handle, const char *source_path, const char *diff_path, const char *target_path);
ADUAPI_LINKAGESPEC size_t CDECL adu_diff_apply_get_error_count(adu_apply_handle handle);
ADUAPI_LINKAGESPEC const char *CDECL adu_diff_apply_get_error_text(adu_apply_handle handle, size_t index);
ADUAPI_LINKAGESPEC int CDECL adu_diff_apply_get_error_code(adu_apply_handle handle, size_t index);

#ifdef __cplusplus
}
#endif