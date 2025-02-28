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

bool hex_digit_to_byte(char hex_digit, unsigned char* byte_value)
{
	if (hex_digit >= '0' && hex_digit <= '9')
	{
		*byte_value = hex_digit - '0';
		return true;
	}
	if (hex_digit >= 'a' && hex_digit <= 'f')
	{
		*byte_value = hex_digit - 'a' + 10;
		return true;
	}
	if (hex_digit >= 'A' && hex_digit <= 'F')
	{
		*byte_value = hex_digit - 'A' + 10;
		return true;
	}
	return false;
}

[[nodiscard]] bool hexstring_to_data(const std::string &hexstring, std::vector<char>& output)
{
	auto input = hexstring.data();
	auto len   = hexstring.size();
	if (hexstring.size() % 2 != 0)
	{
		output.reserve(len / 2 + 1);
		unsigned char byte;
		if (!hex_digit_to_byte(*input, &byte))
		{
			return false;
		}
		output.push_back(byte);
		input++;
		len--;
	}
	else
	{
		output.reserve(len / 2);
	}

	for (size_t i = 0; i < len; i += 2)
	{
		char c1          = hexstring[i];
		char c2          = hexstring[i + 1];
		unsigned char b1;
		unsigned char b2;
		if (!hex_digit_to_byte(c1, &b1) || !hex_digit_to_byte(c2, &b2))
		{
			return false;
		}
		output.push_back((b1 << 4) | b2);
	}
	return true;
}

} // namespace archive_diff::hashing