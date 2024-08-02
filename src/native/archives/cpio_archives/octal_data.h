#pragma once

#include <string_view>

uint32_t get_octal_two_bytes(std::string_view view, size_t offset);
uint32_t get_octal_four_bytes(std::string_view view, size_t offset);

uint64_t octal_char_to_int(char c);
uint64_t octal_characters_to_uint64(
	std::string_view data, size_t offset, size_t length, bool detect_trailing_characters = true);