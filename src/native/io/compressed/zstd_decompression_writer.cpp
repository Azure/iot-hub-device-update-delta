/**
 * @file zstd_decompression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <cstring>

#include "zstd_decompression_writer.h"

namespace archive_diff::io::compressed
{
zstd_decompression_writer::zstd_decompression_writer(std::shared_ptr<io::sequential::writer> &writer) :
	io::sequential::writer_impl(), m_writer(writer)
{
	ZSTD_DCtx_reset(m_zstd_dstream.get(), ZSTD_reset_session_only);

	m_input_data.reserve(ZSTD_DStreamInSize());
	m_output_data.reserve(ZSTD_DStreamOutSize());

	m_input_buffer.src = m_input_data.data();

	m_output_buffer.dst  = m_output_data.data();
	m_output_buffer.size = m_output_data.capacity();
}

zstd_decompression_writer::zstd_decompression_writer(
	std::shared_ptr<io::sequential::writer> &writer, compression_dictionary &&dictionary) :
	zstd_decompression_writer(writer)
{
	m_compression_dictionary = std::move(dictionary);
	ZSTD_DCtx_refPrefix(m_zstd_dstream.get(), m_compression_dictionary.data(), m_compression_dictionary.size());
	ZSTD_DCtx_setParameter(m_zstd_dstream.get(), ZSTD_d_windowLogMax, c_zstd_window_log_max);
}

zstd_decompression_writer::~zstd_decompression_writer() { flush(); }

void zstd_decompression_writer::write_impl(std::string_view buffer)
{
	if (buffer.size() == 0)
	{
		return;
	}

	ZSTD_inBuffer input_buffer{buffer.data(), buffer.size(), 0};

	do
	{
		ZSTD_outBuffer output_buffer{m_output_data.data(), m_output_data.capacity(), 0};

		size_t ret = ZSTD_decompressStream(m_zstd_dstream.get(), &output_buffer, &input_buffer);
		if (ZSTD_isError(ret))
		{
			auto error_name = ZSTD_getErrorName(ret);
			std::string msg = "ZSTD_decompressStream() failed. ret: " + std::to_string(ret)
			                + std::string(" error_name: ") + error_name;
			throw errors::user_exception(errors::error_code::io_zstd_decompress_stream_failed, msg);
		}

		if (output_buffer.pos)
		{
			m_writer->write(std::string_view{static_cast<char *>(output_buffer.dst), output_buffer.pos});
			m_processed_bytes += output_buffer.pos;
		}
	}
	while (input_buffer.size != input_buffer.pos);
}

void zstd_decompression_writer::flush() { m_writer->flush(); }

} // namespace archive_diff::io::compressed