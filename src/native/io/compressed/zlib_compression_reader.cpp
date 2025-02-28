/**
 * @file zlib_compression_reader.cpp
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

#include "zlib_compression_reader.h"

namespace archive_diff::io::compressed
{
const size_t INPUT_DATA_CAPACITY    = 4096;
const int max_compression_mem_level = 9;

zlib_compression_reader::zlib_compression_reader(
	io::reader &reader,
	int compression_level,
	uint64_t uncompressed_input_size,
	uint64_t compressed_result_size,
	zlib_helpers::init_type init_type) :
	m_reader(std::make_unique<io::sequential::basic_reader_wrapper>(reader)), m_compression_level(compression_level),
	m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size),
	m_init_type(init_type)
{
	initialize();
}

zlib_compression_reader::zlib_compression_reader(
	std::unique_ptr<io::sequential::reader> reader,
	int compression_level,
	uint64_t uncompressed_input_size,
	uint64_t compressed_result_size,
	zlib_helpers::init_type init_type) :
	m_reader(std::move(reader)), m_compression_level(compression_level),
	m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size),
	m_init_type(init_type)
{
	initialize();
}

void zlib_compression_reader::initialize()
{
	assert(INPUT_DATA_CAPACITY <= std::numeric_limits<uInt>::max());

	m_input_data.reserve(INPUT_DATA_CAPACITY);

	m_zstr.zalloc   = nullptr;
	m_zstr.zfree    = nullptr;
	m_zstr.opaque   = nullptr;
	m_zstr.next_in  = nullptr;
	m_zstr.avail_in = 0;

	auto init_bits = zlib_helpers::get_init_bits(m_init_type);

	if (deflateInit2(
			&m_zstr, m_compression_level, Z_DEFLATED, init_bits, max_compression_mem_level, Z_DEFAULT_STRATEGY))
	{
		std::string msg = "deflateInit2(): failed.";
		throw errors::user_exception(errors::error_code::io_zlib_reader_init_failed, msg);
	}
}

size_t zlib_compression_reader::read_some(std::span<char> buffer)
{
	auto to_read = static_cast<uInt>(buffer.size());

	m_zstr.next_out  = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(buffer.data()));
	m_zstr.avail_out = to_read;

	uint64_t actual_read = 0;

	while (true)
	{
		bool last_chunk  = m_processed_bytes == m_uncompressed_input_size;
		auto flush_param = last_chunk ? Z_FINISH : Z_NO_FLUSH;

		auto ret = ::deflate(&m_zstr, flush_param);

		// if the state of compression is invalid, then error out
		if (ret == Z_STREAM_ERROR)
		{
			auto msg = "deflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_reader_deflate_failed, msg);
		}

		// If there's an issue with the buffers and we don't have any input for the deflate available,
		// then read some data and make it available. We'll call deflate again.
		if ((ret == Z_BUF_ERROR) && (m_zstr.avail_in == 0))
		{
			auto input_to_read = m_input_data.capacity();
			auto from_reader   = m_reader->read_some(std::span<char>{m_input_data.data(), input_to_read});

			m_zstr.next_in  = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(m_input_data.data()));
			m_zstr.avail_in = static_cast<uInt>(from_reader);

			m_processed_bytes += from_reader;
			continue;
		}

		actual_read = m_zstr.total_out - m_read_offset;

		if (ret == Z_STREAM_END)
		{
			break;
		}

		// If there was no error and we've read everything
		if (ret == Z_OK && (m_processed_bytes == m_uncompressed_input_size))
		{
			break;
		}

		// If we've read as much as we expected and have no error or need more buffers then return
		if (((ret == Z_OK) || ret == (Z_BUF_ERROR)) && (actual_read == to_read))
		{
			break;
		}

		// Otherwise, if we are in some error state, throw an exception
		if (ret != Z_OK)
		{
			auto msg = "deflate() failed. ret: " + std::to_string(ret);
			throw errors::user_exception(errors::error_code::io_zlib_reader_deflate_failed, msg);
		}
	}

	m_read_offset = m_zstr.total_out;

	return actual_read;
}
} // namespace archive_diff::io::compressed