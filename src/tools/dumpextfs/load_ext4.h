/**
 * @file load_ext4.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>

#include "file_details.h"

int load_ext4(const char *ext4_path, std::vector<file_details> *all_files_ptr);
