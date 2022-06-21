/**
 * @file temp_file_backed_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <functional>

#include "readerwriter.h"
#include "binary_file_readerwriter.h"
#include "file.h"

#include "adu_log.h"

namespace io_utility
{
class temp_file_backed_reader : public io_utility::reader
{
	public:
#ifdef WIN32
	temp_file_backed_reader(std::function<void(io_utility::readerwriter &)> populate)
	{
		m_temp_file_path = make_temp_file_path();

		static int x = 0;

		ADU_LOG("temp_file_backed_reader: %d at %s", x++, m_temp_file_path.string().c_str());

		{
			io_utility::binary_file_readerwriter readerwriter(m_temp_file_path);
			populate(readerwriter);
		}

		m_reader = std::make_unique<io_utility::binary_file_reader>(m_temp_file_path);
	}

	virtual ~temp_file_backed_reader()
	{
		m_reader = nullptr;
		fs::remove(m_temp_file_path);
	}

	private:
	fs::path make_temp_file_path()
	{
		const size_t INITIAL_BUFFER_SIZE = 260; // based on MAX_PATH from windows

		std::vector<char> path_value;
		path_value.reserve(INITIAL_BUFFER_SIZE);

		while (true)
		{
			auto ret = tmpnam_s(path_value.data(), path_value.capacity());

			if (ret == 0)
			{
				break;
			}

			path_value.reserve(path_value.capacity() * 2 + 1);
		}

		return fs::path(path_value.data());
	}

	fs::path m_temp_file_path;
#else
	temp_file_backed_reader(std::function<void(io_utility::readerwriter &)> populate)
	{
		FILE *fp = tmpfile();

		if (fp == nullptr)
		{
			std::string msg = "temp_file_backed_reader: tmpfile() failed. errno: " + std::to_string(errno);
			throw error_utility::user_exception(
				error_utility::error_code::io_temp_file_backed_reader_tmpfile_failed, msg);
		}

		auto readerwriter = std::make_unique<io_utility::binary_file_readerwriter>(fp);
		populate(*readerwriter.get());

		m_reader = std::move(readerwriter);
		m_fp.reset(fp);
	}

	virtual ~temp_file_backed_reader()
	{
		m_reader.reset();
		m_fp.reset();
	}

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