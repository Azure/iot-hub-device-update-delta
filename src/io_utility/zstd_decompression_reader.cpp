/**
 * @file zstd_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_decompression_reader.h"

size_t io_utility::zstd_decompression_reader::raw_read_some(gsl::span<char> buffer)
{
	ZSTD_outBuffer out_buffer{buffer.data(), buffer.size(), 0};

	size_t last_ret{};
	bool retry{false};

	do
	{
		size_t ret = ZSTD_decompressStream(m_zstd_dstream.get(), &out_buffer, &m_in_zstd_buffer);

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

			m_in_zstd_buffer.src  = m_in_vector.data();
			m_in_zstd_buffer.size = actual_read;
			m_in_zstd_buffer.pos  = 0;

			bool maybe_incomplete = (actual_read == 0) && (last_ret == ret);

			if (maybe_incomplete && retry)
			{
				throw error_utility::user_exception(error_utility::error_code::io_zstd_decompress_cannot_finish);
			}

			retry = maybe_incomplete;
		}

		last_ret = ret;
	}
	while (out_buffer.size != out_buffer.pos);

	return buffer.size();
}
