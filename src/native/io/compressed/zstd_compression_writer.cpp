/**
 * @file zstd_compression_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <cstring>

#include "zstd_compression_writer.h"

#include "adu_log.h"

namespace archive_diff::io::compressed
{
zstd_compression_writer::zstd_compression_writer(
	std::shared_ptr<io::sequential::writer> &writer, uint64_t level, uint64_t uncompressed_input_size) :
	io::sequential::writer_impl(), m_writer(writer), m_uncompressed_input_size(uncompressed_input_size)
{
	ZSTD_CCtx_setPledgedSrcSize(m_zstd_cstream.get(), m_uncompressed_input_size);
	ZSTD_CCtx_reset(m_zstd_cstream.get(), ZSTD_reset_session_only);
	ZSTD_CCtx_refCDict(m_zstd_cstream.get(), nullptr);
	ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_compressionLevel, (int)level);
	ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_nbWorkers, 1);

	m_input_data.reserve(ZSTD_CStreamInSize());
	m_output_data.reserve(ZSTD_CStreamOutSize());

	m_input_buffer.src = m_input_data.data();

	m_output_buffer.dst  = m_output_data.data();
	m_output_buffer.size = m_output_data.capacity();
}

zstd_compression_writer::zstd_compression_writer(
	std::shared_ptr<io::sequential::writer> &writer,
	uint64_t level,
	uint64_t uncompressed_input_size,
	compression_dictionary &&dictionary) : zstd_compression_writer(writer, level, uncompressed_input_size)
{
	m_compression_dictionary = std::move(dictionary);
	set_dictionary(m_compression_dictionary);
}

void zstd_compression_writer::set_dictionary(const compression_dictionary &dictionary)
{
	ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_enableLongDistanceMatching, 1);
	// set the windowLog to a value larger than the content being compressed
	ZSTD_CCtx_setParameter(
		m_zstd_cstream.get(), ZSTD_c_windowLog, windowLog_from_target_size(m_uncompressed_input_size));

	ZSTD_CCtx_refPrefix(m_zstd_cstream.get(), dictionary.data(), dictionary.size());
}

void zstd_compression_writer::write_impl(std::string_view buffer)
{
	if (buffer.size() == 0)
	{
		return;
	}

	ZSTD_inBuffer input_buffer{buffer.data(), buffer.size(), 0};

	bool done = false;

	auto new_processed_bytes = m_processed_bytes + buffer.size();
	if (new_processed_bytes > m_uncompressed_input_size)
	{
		std::string msg =
			"zstd_compression_writer::write_impl: Processed more bytes than input should have. new_processed_bytes = "
			+ std::to_string(new_processed_bytes)
			+ ", m_uncompressed_input_size = " + std::to_string(m_uncompressed_input_size);

		throw errors::user_exception(errors::error_code::io_zstd_too_much_data_processed, msg);
	}

	bool last_chunk = new_processed_bytes == m_uncompressed_input_size;
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
			throw errors::user_exception(errors::error_code::io_zstd_compressstream2_failed, msg);
		}

		if (output_buffer.pos)
		{
			m_writer->write(std::string_view{static_cast<char *>(output_buffer.dst), output_buffer.pos});
			m_compressed_size += output_buffer.pos;
		}

		done = last_chunk ? (ret == 0) : (input_buffer.size == input_buffer.pos);
	}
	while (!done);

	m_processed_bytes = new_processed_bytes;

	if (m_processed_bytes == m_uncompressed_input_size)
	{
		flush();
	}
}

void zstd_compression_writer::flush() { m_writer->flush(); }
} // namespace archive_diff::io::compressed