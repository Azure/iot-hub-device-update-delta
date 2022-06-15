/**
 * @file zstd_compression_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>

#include "zstd_wrappers.h"
#include "zstd.h"
#include "writer.h"
#include "sequential_writer.h"

namespace io_utility
{
class zstd_compression_writer : public sequential_writer_impl
{
	public:
	zstd_compression_writer(
		sequential_writer *raw_writer,
		uint64_t major_version,
		uint64_t minor_version,
		uint64_t level,
		uint64_t uncompressed_size) :
		sequential_writer_impl(),
		m_raw_writer(raw_writer)
	{
		ZSTD_CCtx_setPledgedSrcSize(m_zstd_cstream.get(), uncompressed_size);
		ZSTD_CCtx_reset(m_zstd_cstream.get(), ZSTD_reset_session_only);
		ZSTD_CCtx_refCDict(m_zstd_cstream.get(), nullptr);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_compressionLevel, (int)level);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_nbWorkers, 1);

		m_uncompressed_size = uncompressed_size;

		m_input_data.reserve(ZSTD_CStreamInSize());
		m_output_data.reserve(ZSTD_CStreamOutSize());

		m_input_buffer.src = m_input_data.data();

		m_output_buffer.dst  = m_output_data.data();
		m_output_buffer.size = m_output_data.capacity();
	}

	void set_dictionary(std::vector<char> *dictionary)
	{
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_enableLongDistanceMatching, 1);
		// set the windowLog to a value larger than the content being compressed
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_windowLog, windowLog_from_target_size(m_uncompressed_size));

		ZSTD_CCtx_refPrefix(m_zstd_cstream.get(), dictionary->data(), dictionary->size());
	}

	virtual ~zstd_compression_writer() = default;

	virtual uint64_t tellp() { return 0; }
	virtual void flush();

	protected:
	virtual void write_impl(std::string_view buffer);

	private:
	uint64_t m_uncompressed_size{};
	uint64_t m_processed_bytes{};

	uint64_t m_compressed_size{};

	sequential_writer *m_raw_writer{};
	unique_zstd_cstream m_zstd_cstream{ZSTD_createCStream()};

	ZSTD_inBuffer m_input_buffer{};
	ZSTD_outBuffer m_output_buffer{};

	std::vector<char> m_input_data;
	std::vector<char> m_output_data;
};
}; // namespace io_utility
