/**
 * @file number_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "user_exception.h"

#include <cstdint>

template <typename NumberT>
NumberT convert_uint64_t(uint64_t number, error_utility::error_code error, const char *msg)
{
	if (number > std::numeric_limits<NumberT>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(error, msg, number);
	}

	return static_cast<NumberT>(number);
}

template <typename NumberTo, typename NumberFrom>
void verify_can_convert_number(NumberFrom number, error_utility::error_code error, const char *msg)
{
	if (number > std::numeric_limits<NumberTo>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(error, msg, number);
	}
}

template <typename NumberTo, typename NumberFrom>
NumberTo convert_number(NumberFrom number, error_utility::error_code error, const char *msg)
{
	verify_can_convert_number<NumberTo, NumberFrom>(number, error, msg);

	return static_cast<NumberTo>(number);
}
