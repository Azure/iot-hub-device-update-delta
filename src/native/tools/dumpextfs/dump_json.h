/**
 * @file dump_json.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include <vector>
#include <iostream>

#include "file_details.h"

void dump_archive(std::ostream &stream, const archive_details &details);
