#include <exception>

#include "hexadecimal_data.h"

uint32_t hexadecimal_characters_to_uint32(std::string_view data, size_t offset, size_t length)
{
	uint32_t value = 0;

	if (data.size() < offset + length)
	{
		throw std::exception();
	}

	for (size_t i = 0; i < length; i++)
	{
		char c                = data[i + offset];
		auto h                = hexadecimal_char_to_int(c);
		uint32_t significance = 1ull << (length - (i + 1)) * 4;
		value += significance * h;
	}

	return value;
}

uint32_t hexadecimal_char_to_int(char c)
{
	if (('0' <= c) && (c <= '9'))
	{
		return c - '0';
	}

	if (('a' <= c) && (c <= 'f'))
	{
		return c - 'a' + 10;
	}

	if (('A' <= c) && (c <= 'F'))
	{
		return c - 'A' + 10;
	}

	throw std::exception();
}

char uint32_to_hexadecimal_character(uint8_t value)
{
	const char *HEXADECIMAL_DIGIT_VALUES = "0123456789ABCDEF";
	return HEXADECIMAL_DIGIT_VALUES[value];
}

void uint32_to_hexadecimal_characters(uint32_t value, std::span<char> buffer)
{
	auto offset = buffer.size() - 1;

	while (true)
	{
		auto current_digit = static_cast<uint8_t>(value % 16);
		buffer[offset]     = uint32_to_hexadecimal_character(current_digit);

		if (offset == 0)
		{
			break;
		}

		value /= 16;
		offset--;
	}
}
