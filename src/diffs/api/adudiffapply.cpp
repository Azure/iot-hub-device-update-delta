/**
 * @file adudiffapply.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <memory>

#include "adudiffapply.h"
#include "apply_session.h"

ADUAPI_LINKAGESPEC adu_apply_handle CDECL adu_diff_apply_create_session()
{
	auto session = std::make_unique<diffs::api::apply_session>();

	return reinterpret_cast<adu_apply_handle>(session.release());
}

ADUAPI_LINKAGESPEC void CDECL adu_diff_apply_close_session(adu_apply_handle handle)
{
	auto session = reinterpret_cast<diffs::api::apply_session *>(handle);

	delete session;
}

ADUAPI_LINKAGESPEC int CDECL adu_diff_apply_set_log_path(adu_apply_handle handle, const char *log_path) { return 0; }

ADUAPI_LINKAGESPEC int CDECL
adu_diff_apply(adu_apply_handle handle, const char *source_path, const char *diff_path, const char *target_path)
{
	auto session = reinterpret_cast<diffs::api::apply_session *>(handle);
	return session->apply(source_path, diff_path, target_path);
}

ADUAPI_LINKAGESPEC size_t CDECL adu_diff_apply_get_error_count(adu_apply_handle handle)
{
	auto session = reinterpret_cast<diffs::api::apply_session *>(handle);
	return session->get_error_count();
}

ADUAPI_LINKAGESPEC const char *CDECL adu_diff_apply_get_error_text(adu_apply_handle handle, size_t index)
{
	auto session = reinterpret_cast<diffs::api::apply_session *>(handle);
	return session->get_error_text(index);
}

ADUAPI_LINKAGESPEC int CDECL adu_diff_apply_get_error_code(adu_apply_handle handle, size_t index)
{
	auto session = reinterpret_cast<diffs::api::apply_session *>(handle);
	return session->get_error_code(index);
}
