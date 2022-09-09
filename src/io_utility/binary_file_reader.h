/**
 * @file binary_file_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"
#include "file.h"

namespace io_utility
{
class binary_file_reader : public reader
{
	public:
	binary_file_reader(const std::string &path);

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer);
	virtual uint64_t size();

	virtual read_style get_read_style() const override { return read_style::random_access; }

	virtual ~binary_file_reader() {}

	private:
	file m_File;
};
} // namespace io_utility