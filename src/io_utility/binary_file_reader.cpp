/**
 * @file binary_file_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "binary_file_reader.h"

#include "user_exception.h"
#include "error_codes.h"

#include "adu_log.h"

io_utility::binary_file_reader::binary_file_reader(const std::string &path) :
	m_File(path, io_utility::file::mode::read, error_utility::error_code::io_binary_file_reader_failed_open)
{}

size_t io_utility::binary_file_reader::read_some(uint64_t offset, gsl::span<char> buffer)
{
	ADU_LOG("binary_file_reader::read_some(). offset=%llu, length=%zu", offset, buffer.size());
	return m_File.read_some(offset, buffer);
}

uint64_t io_utility::binary_file_reader::size() { return m_File.size(); }