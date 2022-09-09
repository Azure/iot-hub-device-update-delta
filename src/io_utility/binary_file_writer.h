/**
 * @file binary_file_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "writer.h"
#include "file.h"

namespace io_utility
{
class binary_file_writer : public writer
{
	public:
	binary_file_writer(const std::string &path);

	virtual void flush();
	virtual void write(uint64_t offset, std::string_view buffer);

	virtual ~binary_file_writer() = default;

	private:
	file m_File;
};
} // namespace io_utility