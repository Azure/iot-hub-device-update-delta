/**
 * @file temp_file_backed_reader_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <io/reader.h>
#include <io/writer.h>

#include <io/file/file.h>
#include <io/io_device.h>

namespace archive_diff::io::file
{
class temp_file
{
	public:
#ifdef WIN32
	temp_file() : m_File(temp_file::tmpfile()) {}
#else
	temp_file() : m_File(tmpfile())
	{
		if (!m_File.is_open())
		{
			std::string msg = "temp_file: tmpfile() failed.";
			throw errors::user_exception(errors::error_code::io_temp_file_tmpfile_failed, msg);
		}
	}
#endif

	virtual ~temp_file() = default;

	size_t read_some(uint64_t offset, std::span<char> buffer) { return m_File.read_some(offset, buffer); }
	uint64_t size() { return m_File.size(); };
	void write(uint64_t offset, std::string_view buffer) { m_File.write(offset, buffer); }
	void flush() { m_File.flush(); }

	private:
#ifdef WIN32
	FILE *tmpfile()
	{
		FILE *fp;

		auto ret = ::tmpfile_s(&fp);
		if (ret != 0)
		{
			std::string msg = "temp_file: tmpfile_s() failed. errno: " + std::to_string(errno);
			throw errors::user_exception(errors::error_code::io_temp_file_tmpfile_failed, msg);
		}

		return fp;
	}
#endif

	file m_File;
};

class temp_file_io_device : public io::io_device
{
	public:
	temp_file_io_device(std::shared_ptr<temp_file> &temp_file) : m_temp_file(temp_file) {}

	virtual ~temp_file_io_device() = default;

	static reader make_reader(std::shared_ptr<temp_file> &temp_file)
	{
		std::shared_ptr<io::io_device> device = std::make_shared<temp_file_io_device>(temp_file);
		return io::reader{device};
	}

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override
	{
		return m_temp_file->read_some(offset, buffer);
	}

	virtual uint64_t size() const override { return m_temp_file->size(); }

	private:
	mutable std::shared_ptr<temp_file> m_temp_file{};
};

class temp_file_writer : public io::writer
{
	public:
	temp_file_writer(std::shared_ptr<temp_file> &temp_file) : m_temp_file(temp_file) {}

	virtual ~temp_file_writer() = default;

	static io::shared_writer make_shared(std::shared_ptr<temp_file> &temp_file)
	{
		return std::make_shared<temp_file_writer>(temp_file);
	}

	virtual void write(uint64_t offset, std::string_view buffer) { m_temp_file->write(offset, buffer); }
	virtual void flush() { m_temp_file->flush(); }
	virtual uint64_t size() const { return m_temp_file->size(); }

	private:
	std::shared_ptr<temp_file> m_temp_file{};
};
} // namespace archive_diff::io::file