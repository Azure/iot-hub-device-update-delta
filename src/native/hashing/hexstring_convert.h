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
} // namespace archive_diff::hashing