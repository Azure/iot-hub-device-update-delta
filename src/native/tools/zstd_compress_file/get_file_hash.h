/**
 * @file get_file_hash.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>

#include <language_support/include_filesystem.h>

std::vector<char> get_file_hash(fs::path path);
