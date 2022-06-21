/**
 * @file zlib_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <vector>
#include <gsl/span>
#include <memory>
#include <algorithm>
#include <string>
#include <limits>

#include <assert.h>

#include "user_exception.h"

#ifdef WIN32
	#include <io.h>
#endif

#include "zlib_decompression_reader.h"

const size_t INBUF_CAPACITY  = 4096;
const size_t OUTBUF_CAPACITY = 4096;

int get_init_bits(io_utility::zlib_decompression_reader::init_type init_type)
{
	switch (init_type)
	{
	case io_utility::zlib_decompression_reader::init_type::raw:
		return -MAX_WBITS;
	case io_utility::zlib_decompression_reader::init_type::gz:
		return 16 + MAX_WBITS;
	}

	std::string msg = "get_init_bits(): Invalid init_type: " + std::to_string(static_cast<int>(init_type));
	throw error_utility::user_exception(error_utility::error_code::io_zlib_init_type_invalid, msg);
}

void io_utility::zlib_decompression_reader::initialize()
{
	if (m_state != state::uninitialized)
	{
		std::string msg = "zlibwrapper_reader::initialize(): Already initialized. Current state: "
		                + std::to_string(static_cast<int>(m_state));
		throw error_utility::user_exception(error_utility::error_code::io_zlib_reader_already_initialized, msg);
	}

	assert(INBUF_CAPACITY <= std::numeric_limits<uInt>::max());

	m_inbuf.reserve(INBUF_CAPACITY);

	m_zstr.zalloc   = nullptr;
	m_zstr.zfree    = nullptr;
	m_zstr.opaque   = nullptr;
	m_zstr.next_in  = nullptr;
	m_zstr.avail_in = 0;

	auto init_bits = get_init_bits(m_init_type);

	if (inflateInit2(&m_zstr, init_bits) != Z_OK)
	{
		std::string msg = "inflateInit2(): failed.";
		throw error_utility::user_exception(error_utility::error_code::io_zlib_reader_init_failed, msg);
	}

	m_state = state::ready;
}

size_t io_utility::zlib_decompression_reader::raw_read_some(gsl::span<char> out_buffer)
{
	if (m_state == state::uninitialized)
	{
		initialize();
	}

	switch (m_state)
	{
	case state::ready:
		break;
	default:
		std::string msg = "zlibwrapper_reader::read_some(): Can't inflate from reader. State has unexpected value: "
		                + std::to_string(static_cast<int>(m_state));
		throw error_utility::user_exception(error_utility::error_code::io_zlib_state_not_ready, msg);
	}

	size_t write_offset = 0;

	size_t remaining_to_write = out_buffer.size();
	while (true)
	{
		m_zstr.next_out = reinterpret_cast<Bytef *>(out_buffer.data() + write_offset);

		uInt avail_out_capacity =
			static_cast<uInt>(std::min(remaining_to_write, (size_t)std::numeric_limits<uInt>::max));
		m_zstr.avail_out = avail_out_capacity;

		if (m_zstr.avail_in == 0)
		{
			auto actual_read = m_sequential_reader->read_some(gsl::span{m_inbuf.data(), m_inbuf.capacity()});
			m_zstr.next_in   = reinterpret_cast<Bytef *>(m_inbuf.data());
			m_zstr.avail_in  = static_cast<uInt>(actual_read);
		}

		int ret = ::inflate(&m_zstr, Z_NO_FLUSH);

		if (ret == Z_OK || ret == Z_STREAM_END)
		{
			if (m_zstr.avail_out < avail_out_capacity)
			{
				size_t written = avail_out_capacity - m_zstr.avail_out;
				write_offset += written;
				remaining_to_write -= written;
			}
		}

		if (ret == Z_STREAM_END)
		{
			break;
		}

		if (ret != Z_OK)
		{
			auto msg = "inflate() failed. ret: " + std::to_string(ret);
			throw error_utility::user_exception(error_utility::error_code::io_zlib_reader_inflate_failed, msg);
		}

		if (remaining_to_write == 0)
		{
			return out_buffer.size();
		}
	}

	if (remaining_to_write)
	{
		std::string msg = "Reached end of deflate stream and couldn't get data.";
		throw error_utility::user_exception(error_utility::error_code::io_zlib_reader_no_more_data, msg);
	}

	inflateEnd(&m_zstr);

	return out_buffer.size();
}