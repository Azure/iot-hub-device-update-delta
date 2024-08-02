/**
 * @file random_data_file.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>

namespace archive_diff::test_utility
{
void create_random_data_file(const std::string &path, uint64_t size);
}