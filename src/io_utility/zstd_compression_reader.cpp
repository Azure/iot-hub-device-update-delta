/**
 * @file zstd_compression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_compression_reader.h"

size_t io_utility::zstd_compression_reader::raw_read_some(gsl::span<char> buffer)
{
	ZSTD_outBuffer out_buffer{buffer.data(), buffer.size(), 0};

	do
	{
		bool last = (m_processed_bytes == m_uncompressed_size);

		size_t ret;

		if (last)
		{
			ret = ZSTD_compressStream2(
				m_zstd_cstream.get(), &out_buffer, &m_in_zstd_buffer, ZSTD_EndDirective::ZSTD_e_end);
		}
		else
		{
			ret = ZSTD_compressStream(m_zstd_cstream.get(), &out_buffer, &m_in_zstd_buffer);
		}

		if (ZSTD_isError(ret))
		{
			throw error_utility::user_exception(
				error_utility::error_code::io_zstd_decompress_stream_failed, ZSTD_getErrorName(ret));
		}

		// There wasn't enough data already present, so read some more
		if ((out_buffer.size != out_buffer.pos) && (m_in_zstd_buffer.size == m_in_zstd_buffer.pos))
		{
			auto to_read     = std::min(ret, m_in_vector.capacity());
			auto actual_read = m_sequential_reader->read_some(gsl::span<char>{m_in_vector.data(), to_read});

			m_in_zstd_buffer.size = actual_read;
			m_in_zstd_buffer.pos  = 0;

			m_processed_bytes += actual_read;
		}
	}
	while (out_buffer.size != out_buffer.pos);

	return buffer.size();
}
