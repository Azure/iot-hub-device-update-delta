/**
 * @file apply_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "user_exception.h"

#include <string>
#include <vector>

namespace diffs
{
namespace api
{
class apply_session
{
	public:
	int apply(const char *source_path, const char *diff_path, const char *target_path);
	size_t get_error_count() const { return errors.size(); }
	const char *get_error_text(size_t index) const
	{
		return (errors.size() < (index + 1)) ? nullptr : errors[index].text.c_str();
	}

	int get_error_code(size_t index) const
	{
		return (errors.size() < (index + 1)) ? 0 : static_cast<int>(errors[index].code);
	}

	private:
	struct error
	{
		error_utility::error_code code;
		std::string text;
	};
	std::vector<error> errors;
};
} // namespace api
} // namespace diffs