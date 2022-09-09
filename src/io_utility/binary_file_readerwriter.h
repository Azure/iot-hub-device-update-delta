/**
 * @file binary_file_readerwriter.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "readerwriter.h"
#include "file.h"

namespace io_utility
{
class binary_file_readerwriter : public readerwriter
{
	public:
	binary_file_readerwriter(const std::string &path);
	binary_file_readerwriter(FILE *fp);

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer);
	virtual read_style get_read_style() const override { return read_style::random_access; }
	virtual uint64_t size();

	virtual void flush();
	virtual void write(uint64_t offset, std::string_view buffer);

	virtual ~binary_file_readerwriter() {}

	private:
	file m_File;
};
} // namespace io_utility