/**
 * @file temp_file_backed_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <functional>

#include "readerwriter.h"
#include "file.h"

namespace io_utility
{
class temp_file_backed_reader : public io_utility::reader
{
	public:
	temp_file_backed_reader(std::function<void(io_utility::readerwriter &)> populate);
	virtual ~temp_file_backed_reader();

	private:
#ifdef WIN32
	std::string make_temp_file_path();
	std::string m_temp_file_path;
#else
	private:
	file::unique_FILE m_fp;
#endif

	public:
	// reader methods
	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer) override
	{
		return m_reader->read_some(offset, buffer);
	}
	virtual read_style get_read_style() const override { return m_reader->get_read_style(); }
	virtual uint64_t size() override { return m_reader->size(); }

	private:
	io_utility::unique_reader m_reader;
};

} // namespace io_utility