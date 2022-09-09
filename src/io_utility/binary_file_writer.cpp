/**
 * @file binary_file_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "binary_file_writer.h"

#include "user_exception.h"

io_utility::binary_file_writer::binary_file_writer(const std::string &path) :
	m_File(path, io_utility::file::mode::write, error_utility::error_code::io_binary_file_writer_failed_open)
{}

void io_utility::binary_file_writer::flush() { m_File.flush(); }

void io_utility::binary_file_writer::write(uint64_t offset, std::string_view buffer) { m_File.write(offset, buffer); }
