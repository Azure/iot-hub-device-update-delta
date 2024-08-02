/**
 * @file zlib_decompression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <limits>
#include <assert.h>

#include "zlib_decompression_writer.h"

namespace archive_diff::io::compressed
{
const size_t OUTPUT_DATA_CAPACITY = 4096;

zlib_decompression_writer::zlib_decompression_writer(
	std::shared_ptr<io::sequential::writer> &writer, zlib_helpers::init_type init_type) :
	io::sequential::writer_impl(),
	m_writer(writer), m_init_type(init_type)
{
	m_output_data.reserve(OUTPUT_DATA_CAPACITY);

	m_zstr.zalloc   = nullptr;
	m_zstr.zfree    = nullptr;
	m_zstr.opaque   = nullptr;
	m_zstr.next_in  = nullptr;
	m_zstr.avail_in = 0;

	auto init_bits = zlib_helpers::get_init_bits(m_init_type);

	if (inflateInit2(&m_zstr, init_bits) != Z_OK)
	{
		std::string msg = "zlib_decompression_writer::zlib_decompression_writer: inflateInit2(): failed.";
		throw errors::user_exception(errors::error_code::io_zlib_writer_init_failed, msg);
	}
}

void zlib_decompression_writer::write_impl(std::string_view buffer)
{
	if (buffer.size() == 0)
	{
		return;
	}

	auto new_processed_bytes = m_processed_bytes + buffer.size();

	m_zstr.next_in  = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(buffer.data()));
	m_zstr.avail_in = static_cast<uInt>(buffer.size());

	while (true)
	{
		reset_output();

		auto ret = ::inflate(&m_zstr, Z_NO_FLUSH);

		if (ret == Z_BUF_ERROR && m_zstr.avail_in == 0)
		{
			break;
		}

		if (ret != Z_OK && ret != Z_STREAM_END)
		{
			auto msg = "zlib_decompression_writer::write_impl: deflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_reader_inflate_failed, msg);
		}

		handle_output();

		if (ret == Z_STREAM_END)
		{
			ret = inflateEnd(&m_zstr);
			if (ret != Z_OK)
			{
				auto msg = "zlib_decompression_writer::write_impl: inflateEnd() failed. ret: " + std::to_string(ret);
				throw errors::user_exception(errors::error_code::io_zlib_writer_inflateEnd_failed, msg);
			}

			break;
		}
	}

	m_processed_bytes = new_processed_bytes;
}

void zlib_decompression_writer::reset_output()
{
	m_zstr.next_out  = reinterpret_cast<Byte *>(m_output_data.data());
	m_zstr.avail_out = static_cast<uInt>(m_output_data.capacity());
}

void zlib_decompression_writer::handle_output()
{
	if (m_uncompressed_bytes < m_zstr.total_out)
	{
		auto available = m_zstr.total_out - m_uncompressed_bytes;

		m_writer->write(std::string_view{m_output_data.data(), static_cast<size_t>(available)});
		m_uncompressed_bytes += available;
	}
}
} // namespace archive_diff::io::compressed