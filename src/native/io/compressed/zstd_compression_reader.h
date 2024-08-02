/**
 * @file zstd_compression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <map>

#include <zstd.h>

#include <io/reader.h>
#include <io/sequential/reader.h>
#include <io/sequential/basic_reader_wrapper.h>

#include "zstd_wrappers.h"

#include "compression_dictionary.h"

namespace archive_diff::io::compressed
{
// We could potentially omit the uncompressed_size, but then we wouldn't know when we're
// done while processing the content in ZSTD. This would mean we wouldn't set ZSTD_EndDirective::ZSTD_e_end
// and the content in the compressed stream would be different, so we require this as a parameter.
class zstd_compression_reader : public io::sequential::reader
{
	public:
	zstd_compression_reader(
		std::unique_ptr<io::sequential::reader> &&reader,
		uint64_t level,
		uint64_t uncompressed_input_size,
		uint64_t compressed_result_size) :
		m_sequential_reader(std::move(reader)),
		m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size)
	{
		initialize(level);
	}

	zstd_compression_reader(
		io::reader &reader, uint64_t level, uint64_t uncompressed_input_size, uint64_t compressed_result_size) :
		m_sequential_reader(std::make_unique<io::sequential::basic_reader_wrapper>(reader)),
		m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size)
	{
		initialize(level);
	}

	zstd_compression_reader(
		std::unique_ptr<io::sequential::reader> &&reader,
		uint64_t level,
		uint64_t uncompressed_input_size,
		uint64_t compressed_result_size,
		compression_dictionary &&dictionary) :
		m_sequential_reader(std::move(reader)),
		m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size)
	{
		initialize(level);

		m_compression_dictionary = std::move(dictionary);
		set_dictionary(m_compression_dictionary);
	}

	zstd_compression_reader(
		io::reader &reader,
		uint64_t level,
		uint64_t uncompressed_input_size,
		uint64_t compressed_result_size,
		compression_dictionary &&dictionary) :
		m_sequential_reader(std::make_unique<io::sequential::basic_reader_wrapper>(reader)),
		m_uncompressed_input_size(uncompressed_input_size), m_compressed_result_size(compressed_result_size)
	{
		initialize(level);

		m_compression_dictionary = std::move(dictionary);
		set_dictionary(m_compression_dictionary);
	}

	virtual ~zstd_compression_reader() = default;

	virtual size_t read_some(std::span<char> buffer) override;

	virtual uint64_t size() const override { return m_compressed_result_size; }

	virtual uint64_t tellg() const override { return m_read_offset; }

	virtual void skip(uint64_t to_skip) override { skip_by_reading(to_skip); }

	private:
	void initialize(uint64_t level)
	{
		ZSTD_CCtx_setPledgedSrcSize(m_zstd_cstream.get(), m_compressed_result_size);
		ZSTD_CCtx_reset(m_zstd_cstream.get(), ZSTD_reset_session_only);
		ZSTD_CCtx_refCDict(m_zstd_cstream.get(), nullptr);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_compressionLevel, (int)level);
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_nbWorkers, 1);

		m_in_vector.reserve(ZSTD_CStreamInSize());

		m_in_zstd_buffer.src  = m_in_vector.data();
		m_in_zstd_buffer.pos  = 0;
		m_in_zstd_buffer.size = 0;
	}

	void set_dictionary(const compression_dictionary &dictionary)
	{
		ZSTD_CCtx_setParameter(m_zstd_cstream.get(), ZSTD_c_enableLongDistanceMatching, 1);
		// set the windowLog to a value larger than the content being compressed
		ZSTD_CCtx_setParameter(
			m_zstd_cstream.get(), ZSTD_c_windowLog, windowLog_from_target_size(m_uncompressed_input_size));

		ZSTD_CCtx_refPrefix(m_zstd_cstream.get(), dictionary.data(), dictionary.size());
	}

	std::unique_ptr<io::sequential::reader> m_sequential_reader;
	compression_dictionary m_compression_dictionary;

	unique_zstd_cstream m_zstd_cstream{ZSTD_createCStream()};

	uint64_t m_uncompressed_input_size{};
	uint64_t m_compressed_result_size{};

	uint64_t m_processed_bytes{};
	uint64_t m_read_offset{};

	ZSTD_inBuffer m_in_zstd_buffer{};

	std::vector<char> m_in_vector;
};
} // namespace archive_diff::io::compressed