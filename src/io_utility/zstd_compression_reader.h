/**
 * @file zstd_compression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <map>

#include "zstd.h"

#include "zstd_wrappers.h"
#include "reader.h"
#include "child_reader.h"
#include "sequential_reader.h"
#include "wrapped_reader_sequential_reader.h"

namespace io_utility
{
class zstd_compression_reader : public sequential_reader
{
	public:
	zstd_compression_reader(
		reader *raw_reader,
		uint64_t major_version,
		uint64_t minor_version,
		uint64_t level,
		uint64_t uncompressed_size,
		uint64_t compressed_size) :
		m_sequential_reader(std::make_unique<wrapped_reader_sequential_reader>(raw_reader)),
		m_uncompressed_size(uncompressed_size), m_compressed_size(compressed_size)
	{
		ZSTD_CCtx_setPledgedSrcSize(m_zstd_cstream.get(), uncompressed_size);
		ZSTD_CCtx_reset(m_zstd_cstream.get(), ZSTD_reset_session_only);
		ZSTD_CCtx_refCDict(m_zstd_cstream.get(), nullptr);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_compressionLevel, (int)level);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_nbWorkers, 1);

		m_in_vector.reserve(ZSTD_CStreamInSize());

		m_in_zstd_buffer.src  = m_in_vector.data();
		m_in_zstd_buffer.pos  = 0;
		m_in_zstd_buffer.size = 0;
	}

	virtual ~zstd_compression_reader() = default;

	virtual size_t raw_read_some(gsl::span<char> buffer) override;

	virtual read_style get_read_style() const override { return read_style::sequential_only; }

	virtual uint64_t size() override { return m_compressed_size; }

	void set_dictionary(std::vector<char> *dictionary)
	{
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_enableLongDistanceMatching, 1);
		// set the windowLog to a value larger than the content being compressed
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_windowLog, windowLog_from_target_size(m_uncompressed_size));

		ZSTD_CCtx_refPrefix(m_zstd_cstream.get(), dictionary->data(), dictionary->size());
	}

	private:
	std::unique_ptr<reader> m_raw_reader;
	std::unique_ptr<sequential_reader> m_sequential_reader;
	unique_zstd_cstream m_zstd_cstream{ZSTD_createCStream()};

	uint64_t m_uncompressed_size{};
	uint64_t m_compressed_size{};

	uint64_t m_processed_bytes{};

	ZSTD_inBuffer m_in_zstd_buffer{};

	std::vector<char> m_in_vector;
};
} // namespace io_utility