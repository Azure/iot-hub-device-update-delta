#pragma once

#include <string_view>
#include <span>

uint32_t hexadecimal_characters_to_uint32(std::string_view data, size_t offset, size_t length);
uint32_t hexadecimal_char_to_int(char c);

void uint32_to_hexadecimal_characters(uint32_t value, std::span<char> buffer);