/**
 * @file user_readerwriter.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "readerwriter.h"
#include "user_readerwriter_pfn.h"

namespace archive_diff::io::user
{
class user_readerwriter : public readerwriter
{
	public:
	using read_some_pfn      = user_readerwriter_read_some_pfn;
	using get_read_style_pfn = user_readerwriter_get_read_style_pfn;
	using size_pfn           = user_readerwriter_size_pfn;
	using write_pfn          = user_readerwriter_write_pfn;
	using flush_pfn          = user_readerwriter_flush_pfn;
	using close_pfn          = user_readerwriter_close_pfn;

	user_readerwriter(void *handle, size_pfn size, write_pfn write, flush_pfn flush, close_pfn close) :
		m_handle(handle), m_read_some(read_none), m_get_read_style(get_no_read_style), m_size(size), m_write(write),
		m_flush(flush), m_close(close)
	{}

	user_readerwriter(
		void *handle,
		read_some_pfn read_some,
		get_read_style_pfn get_read_style,
		size_pfn size,
		write_pfn write,
		flush_pfn flush,
		close_pfn close) :
		m_handle(handle), m_read_some(read_some), m_get_read_style(get_read_style), m_size(size), m_write(write),
		m_flush(flush), m_close(close)
	{}

	virtual ~user_readerwriter() { m_close(m_handle); }

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override
	{
		return m_read_some(m_handle, offset, buffer.data(), buffer.size());
	}

	virtual reader::read_style get_read_style() const override
	{
		return static_cast<reader::read_style>(m_get_read_style(m_handle));
	}

	virtual uint64_t size() const override { return m_size(m_handle); }

	virtual void write(uint64_t offset, std::string_view buffer) override
	{
		// TODO: throw exception when we fail
		m_write(m_handle, offset, buffer.data(), buffer.size());
	}

	virtual void flush() override
	{
		// TODO: throw exception when we fail
		m_flush(m_handle);
	}

	private:
	static size_t read_none(void *, uint64_t, char *, size_t) { return 0; }
	static uint8_t get_no_read_style(void *) { return static_cast<uint8_t>(io::reader::read_style::none); }

	void *m_handle{};

	read_some_pfn m_read_some{};
	get_read_style_pfn m_get_read_style{};
	size_pfn m_size{};
	write_pfn m_write{};
	flush_pfn m_flush{};
	close_pfn m_close{};
};

} // namespace archive_diff::io::user
