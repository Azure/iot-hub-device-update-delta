/**
 * @file zstd_compression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_compression_reader.h"

namespace archive_diff::io::compressed
{
size_t zstd_compression_reader::read_some(std::span<char> buffer)
{
	ZSTD_outBuffer out_buffer{buffer.data(), buffer.size(), 0};

	size_t last_ret{};
	bool retry{false};

	do
	{
		bool last = (m_processed_bytes == m_uncompressed_input_size);

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
			throw errors::user_exception(errors::error_code::io_zstd_compress_stream_failed, ZSTD_getErrorName(ret));
		}

		if (last)
		{
			// positive value here indicates we have remaining bytes to read... but we think we're done?
			if (ret > 0)
			{
				throw errors::user_exception(errors::error_code::io_zstd_compress_finished_early);
			}
			break;
		}

		// There wasn't enough data already present, so read some more
		if ((out_buffer.size != out_buffer.pos) && (m_in_zstd_buffer.size == m_in_zstd_buffer.pos))
		{
			auto to_read     = std::min(ret, m_in_vector.capacity());
			auto from_reader = m_sequential_reader->read_some(std::span<char>{m_in_vector.data(), to_read});

			m_in_zstd_buffer.size = from_reader;
			m_in_zstd_buffer.pos  = 0;

			m_processed_bytes += from_reader;

			bool maybe_incomplete = (from_reader == 0) && (last_ret == ret);

			if (maybe_incomplete && retry)
			{
				throw errors::user_exception(errors::error_code::io_zstd_compress_cannot_finish);
			}

			retry = maybe_incomplete;
		}

		last_ret = ret;
	}
	while (out_buffer.size != out_buffer.pos);

	m_read_offset += out_buffer.pos;

	return out_buffer.pos;
}
} // namespace archive_diff::io::compressed