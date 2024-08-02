/**
 * @file legacy_adudiffapply.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <memory>

#include "legacy_adudiffapply.h"
#include "legacy_apply_session.h"

ADUAPI_LINKAGESPEC adu_apply_handle CDECL adu_diff_apply_create_session()
{
	auto session = std::make_unique<archive_diff::diffs::api::apply_session>();

	return reinterpret_cast<adu_apply_handle>(session.release());
}

ADUAPI_LINKAGESPEC void CDECL adu_diff_apply_close_session(adu_apply_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);

	delete session;
}

ADUAPI_LINKAGESPEC int CDECL
adu_diff_apply_set_log_path([[maybe_unused]] adu_apply_handle handle, [[maybe_unused]] const char *log_path)
{
	return 0;
}

ADUAPI_LINKAGESPEC int CDECL
adu_diff_apply(adu_apply_handle handle, const char *source_path, const char *diff_path, const char *target_path)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->apply(source_path, diff_path, target_path);
}

ADUAPI_LINKAGESPEC uint32_t CDECL adu_diff_apply_get_error_count(adu_apply_handle handle)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->get_error_count();
}

ADUAPI_LINKAGESPEC const char *CDECL adu_diff_apply_get_error_text(adu_apply_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->get_error_text(index);
}

ADUAPI_LINKAGESPEC uint32_t CDECL adu_diff_apply_get_error_code(adu_apply_handle handle, uint32_t index)
{
	auto session = reinterpret_cast<archive_diff::diffs::api::apply_session *>(handle);
	return session->get_error_code(index);
}
