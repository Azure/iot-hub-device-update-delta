/**
 * @file binary_file_readerwriter.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "binary_file_readerwriter.h"

io_utility::binary_file_readerwriter::binary_file_readerwriter(const std::string &path) :
	m_File(path, io_utility::file::mode::read_write, error_utility::error_code::io_binary_file_readerwriter_failed_open)
{}

io_utility::binary_file_readerwriter::binary_file_readerwriter(FILE *fp) : m_File(fp) {}

size_t io_utility::binary_file_readerwriter::read_some(uint64_t offset, gsl::span<char> buffer)
{
	return m_File.read_some(offset, buffer);
}

uint64_t io_utility::binary_file_readerwriter::size() { return m_File.size(); }

void io_utility::binary_file_readerwriter::flush() { m_File.flush(); }

void io_utility::binary_file_readerwriter::write(uint64_t offset, std::string_view buffer)
{
	m_File.write(offset, buffer);
}
