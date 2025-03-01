/**
 * @file zstd_decompression_reader.h
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

#include "compression_dictionary.h"

#include "zstd_wrappers.h"

namespace archive_diff::io::compressed
{
class zstd_decompression_reader : public io::sequential::reader
{
	public:
	zstd_decompression_reader(std::unique_ptr<io::sequential::reader> &&reader, uint64_t uncompressed_size) :
		m_reader(std::move(reader)), m_uncompressed_result_size(uncompressed_size)
	{
		setup_input_buffer();
	}

	zstd_decompression_reader(io::reader &reader, uint64_t uncompressed_size) :
		zstd_decompression_reader(
			std::move(std::make_unique<io::sequential::basic_reader_wrapper>(reader)), uncompressed_size)
	{}

	zstd_decompression_reader(
		std::unique_ptr<io::sequential::reader> &&reader,
		uint64_t uncompressed_size,
		compression_dictionary &&dictionary) : zstd_decompression_reader(std::move(reader), uncompressed_size)
	{
		m_compression_dictionary = std::move(dictionary);
		set_dictionary(m_compression_dictionary);
	}

	zstd_decompression_reader(io::reader &reader, uint64_t uncompressed_size, compression_dictionary &&dictionary) :
		zstd_decompression_reader(
			std::move(std::make_unique<io::sequential::basic_reader_wrapper>(reader)), uncompressed_size)
	{
		m_compression_dictionary = std::move(dictionary);
		set_dictionary(m_compression_dictionary);
	}

	virtual ~zstd_decompression_reader() = default;

	virtual size_t read_some(std::span<char> buffer) override;
	virtual uint64_t size() const override { return m_uncompressed_result_size; }
	virtual uint64_t tellg() const override { return m_read_offset; }
	virtual void skip(uint64_t to_skip) override { skip_by_reading(to_skip); }

	protected:
	void setup_input_buffer()
	{
		m_in_vector.reserve(ZSTD_DStreamInSize());

		m_in_zstd_buffer.src  = m_in_vector.data();
		m_in_zstd_buffer.pos  = 0;
		m_in_zstd_buffer.size = 0;
	}

	void set_dictionary(const compression_dictionary &dictionary)
	{
		ZSTD_DCtx_refPrefix(m_zstd_dstream.get(), dictionary.data(), dictionary.size());
		ZSTD_DCtx_setParameter(m_zstd_dstream.get(), ZSTD_d_windowLogMax, c_zstd_window_log_max);
	}

	private:
	std::unique_ptr<io::sequential::reader> m_reader;
	compression_dictionary m_compression_dictionary;

	unique_zstd_dstream m_zstd_dstream{ZSTD_createDStream()};

	uint64_t m_read_offset{};
	uint64_t m_uncompressed_result_size{};

	ZSTD_inBuffer m_in_zstd_buffer{};

	std::vector<char> m_in_vector;
};
} // namespace archive_diff::io::compressed