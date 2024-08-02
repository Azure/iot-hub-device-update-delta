/**
 * @file session_base.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <errors/user_exception.h>

#include <string>
#include <vector>

namespace archive_diff::diffs
{
namespace api
{
class session_base
{
	public:
	uint32_t get_error_count() const { return static_cast<uint32_t>(m_errors.size()); }
	const char *get_error_text(uint32_t index) const
	{
		return (m_errors.size() < (index + 1)) ? nullptr : m_errors[index].text.c_str();
	}

	uint32_t get_error_code(uint32_t index) const
	{
		return (m_errors.size() < (index + 1)) ? 0 : static_cast<int>(m_errors[index].code);
	}

	protected:
	void clear_errors() { m_errors.clear(); }

	void add_error(errors::error_code code, std::string_view text)
	{
		m_errors.emplace_back(error{code, std::string(text)});
	}

	private:
	struct error
	{
		errors::error_code code;
		std::string text;
	};
	std::vector<error> m_errors;
};
} // namespace api
} // namespace archive_diff::diffs