/**
 * @file zstd_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_decompression_reader.h"

namespace archive_diff::io::compressed
{
size_t zstd_decompression_reader::read_some(std::span<char> buffer)
{
	ZSTD_outBuffer output_buffer{buffer.data(), buffer.size(), 0};

	size_t last_ret{};
	bool retry{false};

	size_t total_read = 0;

	do
	{
		size_t ret = ZSTD_decompressStream(m_zstd_dstream.get(), &output_buffer, &m_in_zstd_buffer);

		if (ZSTD_isError(ret))
		{
			auto error_name = ZSTD_getErrorName(ret);
			std::string msg = "ZSTD_decompressStream() failed. ret: " + std::to_string(ret)
			                + std::string(" error_name: ") + error_name;
			throw errors::user_exception(errors::error_code::io_zstd_decompress_stream_failed, msg);
		}

		total_read = output_buffer.pos;

		if ((total_read + m_read_offset) == m_uncompressed_result_size)
		{
			break;
		}

		// There wasn't enough data already present, so read some more
		if ((output_buffer.size != output_buffer.pos) && (m_in_zstd_buffer.size == m_in_zstd_buffer.pos))
		{
			auto to_read     = std::min(ret, m_in_vector.capacity());
			auto actual_read = m_reader->read_some(std::span<char>{m_in_vector.data(), to_read});

			m_in_zstd_buffer.src  = m_in_vector.data();
			m_in_zstd_buffer.size = actual_read;
			m_in_zstd_buffer.pos  = 0;

			bool maybe_incomplete = (actual_read == 0) && (last_ret == ret);

			if (maybe_incomplete && retry)
			{
				throw errors::user_exception(errors::error_code::io_zstd_decompress_cannot_finish);
			}

			retry = maybe_incomplete;
		}

		last_ret = ret;
	}
	while (output_buffer.size != output_buffer.pos);

	m_read_offset += total_read;

	return total_read;
}
} // namespace archive_diff::io::compressed