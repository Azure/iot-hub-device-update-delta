/**
 * @file binary_file_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "binary_file_writer.h"

#include "user_exception.h"

namespace archive_diff::io::file
{
binary_file_writer::binary_file_writer(const std::string &path) :
	m_File(path, file::mode::write, errors::error_code::io_binary_file_writer_failed_open)
{}

void binary_file_writer::flush() { m_File.flush(); }

void binary_file_writer::write(uint64_t offset, std::string_view buffer) { m_File.write(offset, buffer); }

uint64_t binary_file_writer::size() const { return m_File.size(); }
} // namespace archive_diff::io::file