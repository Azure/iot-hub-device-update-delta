/**
 * @file dump_json.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include <vector>

#include "file_details.h"

void dump_all_files(FILE *stream, const std::vector<file_details> &all_files);
