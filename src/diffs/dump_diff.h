/**
 * @file dump_diff.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <iostream>

namespace diffs
{
void dump_diff(const std::string &diff_path, std::ostream &ostream);
}
