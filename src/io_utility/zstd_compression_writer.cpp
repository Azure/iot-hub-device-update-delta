/**
 * @file zstd_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <cstring>

#include "zstd_compression_writer.h"

void io_utility::zstd_compression_writer::write_impl(std::string_view buffer)
{
	if (buffer.size() == 0)
	{
		return;
	}

	ZSTD_inBuffer input_buffer{buffer.data(), buffer.size(), 0};

	bool done = false;

	auto new_processed_bytes = m_processed_bytes + buffer.size();
	bool last_chunk          = new_processed_bytes == m_uncompressed_size;
	do
	{
		ZSTD_outBuffer output_buffer{m_output_data.data(), m_output_data.capacity(), 0};

		auto op = last_chunk ? ZSTD_EndDirective::ZSTD_e_end : ZSTD_EndDirective::ZSTD_e_continue;

		size_t ret = ZSTD_compressStream2(m_zstd_cstream.get(), &output_buffer, &input_buffer, op);
		if (ZSTD_isError(ret))
		{
			auto error_name = ZSTD_getErrorName(ret);
			std::string msg = "ZSTD_compressStream2() failed. ret: " + std::to_string(ret)
			                + std::string(" error_name: ") + error_name;
			throw error_utility::user_exception(error_utility::error_code::io_zstd_compressstream2_failed, msg);
		}

		if (output_buffer.pos)
		{
			m_raw_writer->write(std::string_view{static_cast<char *>(output_buffer.dst), output_buffer.pos});
			m_compressed_size += output_buffer.pos;
		}

		done = last_chunk ? (ret == 0) : (input_buffer.size == input_buffer.pos);
	}
	while (!done);

	m_processed_bytes = new_processed_bytes;

	if (m_processed_bytes == m_uncompressed_size)
	{
		flush();
	}
}

void io_utility::zstd_compression_writer::flush() { m_raw_writer->flush(); }
