/**
 * @file compression_util.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <language_support/include_filesystem.h>

void zstd_uncompress_file(fs::path source, fs::path target);
