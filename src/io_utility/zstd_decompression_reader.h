/**
 * @file zstd_decompression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <map>

#include "zstd_wrappers.h"
#include "zstd.h"
#include "reader.h"
#include "child_reader.h"
#include "sequential_reader.h"
#include "wrapped_reader_sequential_reader.h"

namespace io_utility
{
class zstd_decompression_reader : public sequential_reader
{
	public:
	zstd_decompression_reader(reader *reader, uint64_t uncompressed_size) :
		m_uncompressed_size(uncompressed_size),
		m_sequential_reader(std::make_unique<wrapped_reader_sequential_reader>(reader))
	{
		setup_input_buffer();
	}

	zstd_decompression_reader(std::unique_ptr<reader> &&reader, uint64_t uncompressed_size) :
		m_uncompressed_size(uncompressed_size), m_raw_reader(std::move(reader)),
		m_sequential_reader(std::make_unique<wrapped_reader_sequential_reader>(m_raw_reader.get()))
	{
		setup_input_buffer();
	}

	zstd_decompression_reader(reader *reader, uint64_t uncompressed_size, io_utility::reader *delta_basis_reader) :
		zstd_decompression_reader(reader, uncompressed_size)
	{
		setup_dictionary(delta_basis_reader);
	}

	zstd_decompression_reader(
		std::unique_ptr<reader> &&reader, uint64_t uncompressed_size, io_utility::reader *delta_basis_reader) :
		zstd_decompression_reader(std::move(reader), uncompressed_size)
	{
		setup_dictionary(delta_basis_reader);
	}

	virtual ~zstd_decompression_reader() = default;

	virtual size_t raw_read_some(gsl::span<char> buffer) override;

	virtual read_style get_read_style() const override { return read_style::sequential_only; }

	virtual uint64_t size() override { return m_uncompressed_size; }

	protected:
	void setup_input_buffer()
	{
		m_in_vector.reserve(ZSTD_DStreamInSize());

		m_in_zstd_buffer.src  = m_in_vector.data();
		m_in_zstd_buffer.pos  = 0;
		m_in_zstd_buffer.size = 0;
	}

	void setup_dictionary(reader *dictionary_reader)
	{
		std::vector<char> dictionary;

		if (dictionary_reader->size() > std::numeric_limits<gsl::span<char>::size_type>::max())
		{
			std::string msg =
				"zstd_decompression_reader::setup_dictionary(): Attempting to set dictionary with size over span "
				"size_type max. "
				" reader size: "
				+ std::to_string(dictionary_reader->size())
				+ ", size_type::max(): " + std::to_string(std::numeric_limits<gsl::span<char>::size_type>::max());

			throw error_utility::user_exception(error_utility::error_code::io_zstd_dictionary_too_large, msg);
		}

		auto dictionary_size = static_cast<gsl::span<char>::size_type>(dictionary_reader->size());

		dictionary.reserve(dictionary_size);

		dictionary_reader->read(0, gsl::span{dictionary.data(), dictionary_size});

		ZSTD_DCtx_refPrefix(m_zstd_dstream.get(), dictionary.data(), dictionary_size);
		ZSTD_DCtx_setParameter(m_zstd_dstream.get(), ZSTD_d_windowLogMax, c_zstd_window_log_max);
	}

	void set_dictionary(std::vector<char> *dictionary)
	{
		ZSTD_DCtx_refPrefix(m_zstd_dstream.get(), dictionary->data(), dictionary->size());
		ZSTD_DCtx_setParameter(m_zstd_dstream.get(), ZSTD_d_windowLogMax, c_zstd_window_log_max);
	}

	private:
	std::unique_ptr<reader> m_raw_reader;
	std::unique_ptr<sequential_reader> m_sequential_reader;

	unique_zstd_dstream m_zstd_dstream{ZSTD_createDStream()};

	uint64_t m_uncompressed_size{};

	ZSTD_inBuffer m_in_zstd_buffer{};

	std::vector<char> m_in_vector;
};
} // namespace io_utility