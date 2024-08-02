/**
 * @file reader_validation.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <io/reader.h>

#include <language_support/include_filesystem.h>

namespace archive_diff::test_utility
{
bool reader_and_file_are_equal(
	io::reader &reader, fs::path file_path, uint64_t offset, uint64_t length, size_t chunk_size);

bool files_are_equal(fs::path left, fs::path right);
} // namespace archive_diff::test_utility