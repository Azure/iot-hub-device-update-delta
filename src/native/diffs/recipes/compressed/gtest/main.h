/**
 * @file main.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <language_support/include_filesystem.h>

extern fs::path g_test_data_root;

const fs::path c_sample_file_zst_compressed        = "sample.zst";
const size_t c_sample_file_zst_uncompressed_size   = 291328;
const uint64_t c_sample_file_zst_compression_level = 3;

const fs::path c_sample_file_deflate_compressed   = "remainder.dat.deflate";
const fs::path c_sample_file_deflate_uncompressed = "remainder.dat";