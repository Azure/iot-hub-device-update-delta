/**
 * @file user_exception.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>

#include "error_codes.h"

#include <vector>

namespace error_utility
{
class user_exception
{
	public:
	user_exception(error_code error);
	user_exception(error_code error, std::string msg);

	error_code get_error() const { return m_error; }
	const char *get_message() const { return m_msg.c_str(); }

	static user_exception value_exceeds_size_t(error_code error, std::string msg, uint64_t value);

	private:
	void populate_callstack();

	std::vector<std::string> m_callstack;
	error_code m_error{};
	std::string m_msg;
};
} // namespace error_utility