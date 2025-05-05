/**
 * @file io_device.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "io_device.h"

#include <string>
#include <iostream>

#include "user_exception.h"
#include "error_codes.h"
#include "adu_log.h"

namespace archive_diff::io::file
{
io_device::io_device(const std::string &path) :
	m_File(path, file::mode::read, errors::error_code::io_binary_file_reader_failed_open)
{}

reader io_device::make_reader(const std::string &path)
{
	std::shared_ptr<io::io_device> device = std::make_shared<io_device>(path);
	return io::reader{device};
}

size_t io_device::read_some(uint64_t offset, std::span<char> buffer) { return m_File.read_some(offset, buffer); }

uint64_t io_device::size() const { return m_File.size(); }
} // namespace archive_diff::io::file