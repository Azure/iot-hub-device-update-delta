/**
 * @file zlib_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <vector>
#include <span>
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

namespace archive_diff::io::compressed
{
const size_t INPUT_DATA_CAPACITY = 4096;

void zlib_decompression_reader::initialize()
{
	assert(INPUT_DATA_CAPACITY <= std::numeric_limits<uInt>::max());

	m_input_data.reserve(INPUT_DATA_CAPACITY);

	m_zstr.zalloc   = nullptr;
	m_zstr.zfree    = nullptr;
	m_zstr.opaque   = nullptr;
	m_zstr.next_in  = nullptr;
	m_zstr.avail_in = 0;

	auto init_bits = zlib_helpers::get_init_bits(m_init_type);

	if (inflateInit2(&m_zstr, init_bits) != Z_OK)
	{
		std::string msg = "inflateInit2(): failed.";
		throw errors::user_exception(errors::error_code::io_zlib_reader_init_failed, msg);
	}
}

size_t io::compressed::zlib_decompression_reader::read_some(std::span<char> output_buffer)
{
	size_t remaining_to_write = output_buffer.size();
	uint64_t total_read{0};

	while (true)
	{
		m_zstr.next_out = reinterpret_cast<Bytef *>(output_buffer.data() + total_read);

		uInt avail_out_capacity =
			static_cast<uInt>(std::min(remaining_to_write, (size_t)std::numeric_limits<uInt>::max));
		m_zstr.avail_out = avail_out_capacity;

		if (m_zstr.avail_in == 0)
		{
			auto actual_read = m_reader->read_some(std::span{m_input_data.data(), m_input_data.capacity()});
			m_zstr.next_in   = reinterpret_cast<Bytef *>(m_input_data.data());
			m_zstr.avail_in  = static_cast<uInt>(actual_read);
		}

		int ret = ::inflate(&m_zstr, Z_NO_FLUSH);

		if (ret == Z_OK || ret == Z_STREAM_END)
		{
			if (m_zstr.avail_out < avail_out_capacity)
			{
				size_t written = avail_out_capacity - m_zstr.avail_out;
				remaining_to_write -= written;
				total_read += written;
			}
		}

		if (ret == Z_STREAM_END)
		{
			inflateEnd(&m_zstr);
			break;
		}

		if (ret != Z_OK)
		{
			auto msg = "inflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_reader_inflate_failed, msg);
		}

		if (remaining_to_write == 0)
		{
			break;
		}
	}

	m_read_offset += total_read;
	return total_read;
}
} // namespace archive_diff::io::compressed