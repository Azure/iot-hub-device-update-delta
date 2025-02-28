/**
 * @file hexstring_convert.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>

#include "algorithm.h"

namespace archive_diff::hashing
{
std::string data_to_hexstring(const char *data, size_t byte_count);
[[nodiscard]] bool hexstring_to_data(const std::string &hexstring, std::vector<char>& output);
} // namespace archive_diff::hashing