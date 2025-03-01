/**
 * @file zlib_decompression_reader.h
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
class zlib_decompression_reader : public io::sequential::reader
{
	public:
	zlib_decompression_reader(io::reader &reader, uint64_t uncompressed_size, zlib_helpers::init_type init_type) :
		m_reader(std::make_unique<io::sequential::basic_reader_wrapper>(reader)),
		m_uncompressed_size(uncompressed_size), m_init_type(init_type)
	{
		initialize();
	}

	zlib_decompression_reader(
		std::unique_ptr<io::sequential::reader> reader, uint64_t uncompressed_size, zlib_helpers::init_type init_type) :
		m_reader(std::move(reader)), m_uncompressed_size(uncompressed_size), m_init_type(init_type)
	{
		initialize();
	}

	virtual ~zlib_decompression_reader() = default;

	virtual size_t read_some(std::span<char> buffer) override;
	virtual uint64_t size() const override { return m_uncompressed_size; }
	virtual void skip(uint64_t to_skip) override { skip_by_reading(to_skip); }
	virtual uint64_t tellg() const override { return m_read_offset; }

	private:
	void initialize();

	std::unique_ptr<io::sequential::reader> m_reader;
	uint64_t m_uncompressed_size{};
	zlib_helpers::init_type m_init_type{zlib_helpers::init_type::raw};

	auto_zlib_dstream m_zstr{};
	uint64_t m_read_offset{};
	std::vector<char> m_input_data;
};
} // namespace archive_diff::io::compressed