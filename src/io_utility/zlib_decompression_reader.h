/**
 * @file zlib_decompression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>

#include "zlib.h"
#include "reader.h"
#include "sequential_reader.h"
#include "wrapped_reader_sequential_reader.h"
#include "child_reader.h"

namespace io_utility
{
class zlib_decompression_reader : public sequential_reader
{
	public:
	enum class init_type
	{
		raw,
		gz
	};

	zlib_decompression_reader(io_utility::reader *reader, uint64_t uncompressed_size, init_type init_type) :
		m_uncompressed_size(uncompressed_size), m_init_type(init_type)
	{
		m_sequential_reader = std::make_unique<wrapped_reader_sequential_reader>(reader);
	}

	zlib_decompression_reader(
		std::unique_ptr<io_utility::reader> &&reader, uint64_t uncompressed_size, init_type init_type) :
		m_uncompressed_size(uncompressed_size),
		m_init_type(init_type)
	{
		m_raw_reader_storage = std::move(reader);
		m_sequential_reader  = std::make_unique<wrapped_reader_sequential_reader>(m_raw_reader_storage.get());
	}

	enum class state
	{
		uninitialized = 0,
		ready         = 1,
		done          = 2,
	};
	virtual ~zlib_decompression_reader() = default;

	virtual size_t raw_read_some(gsl::span<char> buffer);

	virtual uint64_t size() { return m_uncompressed_size; }

	state get_state() { return m_state; }

	private:
	void initialize();

	init_type m_init_type{init_type::raw};

	std::unique_ptr<reader> m_raw_reader_storage;
	std::unique_ptr<sequential_reader> m_sequential_reader;
	uint64_t m_uncompressed_size{};

	state m_state{state::uninitialized};
	z_stream m_zstr{};

	std::vector<char> m_inbuf;
};
} // namespace io_utility