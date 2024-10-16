/**
 * @file hexstring_convert.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */


#include "hexstring_convert.h"

namespace archive_diff::hashing
{
std::string data_to_hexstring(const char *data, size_t byte_count)
{
	const char *hex_digits = "0123456789abcdef";
	std::string hex_string;

	for (size_t i = 0; i < byte_count; i++)
	{
		unsigned char b = data[i];
		hex_string += hex_digits[b / 16];
		hex_string += hex_digits[b % 16];
	}
	return hex_string;
}
} // namespace archive_diff::hashing