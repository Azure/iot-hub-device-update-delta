/**
 * @file get_file_hash.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

std::vector<char> get_file_hash(fs::path path);
