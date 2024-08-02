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
std::string hash_to_hexstring(const char *hash, hashing::algorithm alg);
} // namespace archive_diff::hashing