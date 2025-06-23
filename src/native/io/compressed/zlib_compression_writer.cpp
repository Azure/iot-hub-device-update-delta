/**
 * @file zlib_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <limits>
#include <assert.h>

#include <fmt/core.h>

#include <adu_log.h>

#include "zlib_compression_writer.h"

namespace archive_diff::io::compressed
{
const size_t OUTPUT_DATA_CAPACITY = 4096;

zlib_compression_writer::zlib_compression_writer(
	std::shared_ptr<io::sequential::writer> &writer, int level, zlib_helpers::init_type init_type) :
	io::sequential::writer_impl(), m_writer(writer), m_compression_level(level), m_init_type(init_type)
{
	m_output_data.reserve(OUTPUT_DATA_CAPACITY);

	m_zstr.zalloc   = nullptr;
	m_zstr.zfree    = nullptr;
	m_zstr.opaque   = nullptr;
	m_zstr.next_in  = nullptr;
	m_zstr.avail_in = 0;

	auto init_bits = zlib_helpers::get_init_bits(m_init_type);

	const int max_compression_mem_level = 9;
	if (deflateInit2(&m_zstr, m_compression_level, Z_DEFLATED, init_bits, max_compression_mem_level, Z_DEFAULT_STRATEGY)
	    != Z_OK)
	{
		std::string msg = "deflateInit2(): failed.";
		throw errors::user_exception(errors::error_code::io_zlib_reader_init_failed, msg);
	}

	if (init_type == zlib_helpers::init_type::gz)
	{
		zlib_helpers::initialize_header(m_gz_header);

		auto ret = deflateSetHeader(&m_zstr, &m_gz_header);
		if (ret)
		{
			std::string msg = fmt::format("deflateSetHeader(): failed. ret: {}", ret);
			throw errors::user_exception(errors::error_code::io_zlib_writer_deflate_set_header_failed, msg);
		}
	}
}

void zlib_compression_writer::write_impl(std::string_view buffer)
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

		// ADU_LOG(
		//	"zlib_compression_writer::write_impl: Calling deflate. m_zstr.avail_in: {}, m_processed_bytes: {}",
		//	m_zstr.avail_in,
		//	m_processed_bytes);

		auto ret = deflate(&m_zstr, Z_NO_FLUSH);

		if (ret == Z_BUF_ERROR && m_zstr.avail_in == 0)
		{
			break;
		}

		if (ret != Z_OK && ret != Z_STREAM_END)
		{
			auto msg = "zlib_compression_writer::write_impl: deflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_writer_deflate_failed, msg);
		}

		handle_output();

		if (ret == Z_STREAM_END)
		{
			break;
		}
	}

	m_processed_bytes = new_processed_bytes;
}

void zlib_compression_writer::stream_end()
{
	while (true)
	{
		reset_output();

		ADU_LOG(
			"zlib_compression_writer::write_impl: Calling deflate with Z_FINISH. m_zstr.avail_in: {}, "
			"m_processed_bytes: {}",
			m_zstr.avail_in,
			m_processed_bytes);

		auto ret = deflate(&m_zstr, Z_FINISH);

		if (ret != Z_OK && ret != Z_STREAM_END)
		{
			auto msg = "zlib_compression_writer::stream_end: deflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_writer_deflate_failed, msg);
		}

		handle_output();

		if (ret == Z_STREAM_END)
		{
			break;
		}
	}
}

void zlib_compression_writer::reset_output()
{
	m_zstr.next_out  = reinterpret_cast<Byte *>(m_output_data.data());
	m_zstr.avail_out = static_cast<uInt>(m_output_data.capacity());
}

void zlib_compression_writer::handle_output()
{
	if (m_compressed_output_size < m_zstr.total_out)
	{
		auto available = m_zstr.total_out - m_compressed_output_size;

		m_writer->write(std::string_view{m_output_data.data(), static_cast<size_t>(available)});
		m_compressed_output_size += available;
	}
}

void zlib_compression_writer::flush()
{
	stream_end();
	m_writer->flush();
}
} // namespace archive_diff::io::compressed