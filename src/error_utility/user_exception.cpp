/**
 * @file user_exception.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "user_exception.h"

#include <limits>

#ifdef __linux__
	#include <execinfo.h>
#endif

error_utility::user_exception::user_exception(error_code error) : user_exception(error, "") {}
error_utility::user_exception::user_exception(error_code error, std::string msg) : m_error(error), m_msg(msg)
{
	populate_callstack();

	if (m_callstack.size() != 0)
	{
		m_msg += "\nCallstack:";
		for (const auto &symbol : m_callstack)
		{
			m_msg += "\n\t";
			m_msg += symbol;
		}
	}
}

void error_utility::user_exception::populate_callstack()
{
#ifdef __linux__
	std::vector<void *> stack_addresses;
	stack_addresses.reserve(10);

	int address_count;
	while (true)
	{
		address_count = backtrace(stack_addresses.data(), stack_addresses.capacity());

		if (address_count < stack_addresses.capacity())
		{
			break;
		}

		auto new_capacity = stack_addresses.capacity() * 2 + 1;
		stack_addresses.reserve(new_capacity);
	}

	char **stack_symbols = backtrace_symbols(stack_addresses.data(), address_count);
	if (stack_symbols == nullptr)
	{
		return;
	}

	for (int i = 0; i < address_count; i++)
	{
		m_callstack.emplace_back(std::string(stack_symbols[i]));
	}

	free(stack_symbols);
#endif
}

error_utility::user_exception error_utility::user_exception::value_exceeds_size_t(
	error_code error, std::string msg, uint64_t value)
{
	std::string full_msg = msg;
	msg += " Value: " + std::to_string(value);
	msg += ", size_t max: " + std::to_string(std::numeric_limits<size_t>::max());

	return user_exception(error, full_msg);
}
