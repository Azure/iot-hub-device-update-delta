/**
 * @file user_exception.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "user_exception.h"

#include <limits>

error_utility::user_exception error_utility::user_exception::value_exceeds_size_t(
	error_code error, std::string msg, uint64_t value)
{
	std::string full_msg = msg;
	msg += " Value: " + std::to_string(value);
	msg += ", size_t max: " + std::to_string(std::numeric_limits<size_t>::max());

	return user_exception(error, full_msg);
}
