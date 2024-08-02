/**
 * @file common.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <memory>
#include <vector>

#include <io/reader.h>

const size_t c_reader_vector_size = 100;

void test_buffer_reader(std::shared_ptr<std::vector<char>> data, archive_diff::io::reader &reader);