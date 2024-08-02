/**
 * @file legacy_apply_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <errors/user_exception.h>

#include <string>
#include <vector>

#include "session_base.h"

namespace archive_diff::diffs
{
namespace api
{
class apply_session : public session_base
{
	public:
	uint32_t apply(const char *source_path, const char *diff_path, const char *target_path);
};
} // namespace api
} // namespace archive_diff::diffs