#include <exception>

#include "octal_data.h"

const uint32_t OCTAL_UNIT_VALUE = 0x100u;

uint32_t get_octal_two_bytes(std::string_view view, size_t offset)
{
	if (view.size() < (offset + 2))
	{
		throw std::exception();
	}

	uint32_t value = view[offset] + (OCTAL_UNIT_VALUE * view[offset + 1]);

	return value;
}

uint32_t get_octal_four_bytes(std::string_view view, size_t offset)
{
	if (view.size() < (offset + 4))
	{
		throw std::exception();
	}

	uint32_t value = view[offset + 2] + (OCTAL_UNIT_VALUE * view[offset + 3]) + ((OCTAL_UNIT_VALUE << 4) * view[offset])
	               + ((OCTAL_UNIT_VALUE << 8) * view[offset + 1]);

	return value;
}

bool is_trailing_character(char c) { return c == 0 || c == ' '; }

uint64_t octal_char_to_int(char c)
{
	if ((c < '0') || (c > '7'))
	{
		throw std::exception();
	}

	return c - '0';
}

uint64_t octal_characters_to_uint64(
	std::string_view data, size_t offset, size_t length, bool detect_trailing_characters)
{
	uint64_t value = 0;

	if (detect_trailing_characters)
	{
		for (; length >= 1; length--)
		{
			auto last_byte = data[offset + length - 1];
			if (!is_trailing_character(last_byte))
			{
				break;
			}
		}
	}

	if (!length)
	{
		return 0;
	}

	for (size_t i = 0; i < length; i++)
	{
		char c                = data[offset + i];
		auto o                = octal_char_to_int(c);
		uint64_t significance = 1ull << ((length - (i + 1)) * 3);
		value += o * significance;
	}

	return value;
}