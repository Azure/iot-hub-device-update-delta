/**
 * @file dump_diff.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

namespace diffs
{
void dump_diff(fs::path diff_path, std::ostream &ostream);
}
