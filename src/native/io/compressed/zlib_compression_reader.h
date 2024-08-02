/**
 * @file zlib_compression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>

#include <zlib.h>

#include <io/reader.h>
#include <io/sequential/reader.h>
#include <io/sequential/basic_reader_wrapper.h>

#include "zlib_helpers.h"

namespace archive_diff::io::compressed
{
using zlib_init_type = io::compressed::zlib_helpers::init_type;

class zlib_compression_reader : public io::sequential::reader
{
	public:
	zlib_compression_reader(
		io::reader &reader,
		int compression_level,
		uint64_t uncompressed_input_size,
		uint64_t compressed_result_size,
		zlib_helpers::init_type init_type);

	zlib_compression_reader(
		std::unique_ptr<io::sequential::reader> reader,
		int compression_level,
		uint64_t uncompressed_input_size,
		uint64_t compressed_result_size,
		zlib_helpers::init_type init_type);

	virtual ~zlib_compression_reader() = default;

	virtual size_t read_some(std::span<char> buffer) override;
	virtual uint64_t size() const override { return m_compressed_result_size; }
	virtual void skip(uint64_t to_skip) override { skip_by_reading(to_skip); }
	virtual uint64_t tellg() const override { return m_read_offset; }

	private:
	void initialize();

	std::unique_ptr<io::sequential::reader> m_reader;
	int m_compression_level;
	uint64_t m_uncompressed_input_size{};
	uint64_t m_compressed_result_size{};
	zlib_helpers::init_type m_init_type{zlib_helpers::init_type::raw};

	uint64_t m_processed_bytes{};
	uint64_t m_read_offset{};

	auto_zlib_cstream m_zstr{};

	std::vector<char> m_input_data;
};
} // namespace archive_diff::io::compressed