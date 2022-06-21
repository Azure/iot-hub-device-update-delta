/**
 * @file sequential_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"

namespace io_utility
{
class sequential_reader : public reader
{
	public:
	virtual ~sequential_reader() = default;

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer);

	template <typename T>
	void read(T *value)
	{
		read(gsl::span{reinterpret_cast<char *>(value), sizeof(T)});
	}

	virtual void skip(uint64_t to_skip);

	virtual size_t read_some(gsl::span<char> buffer);

	void read(gsl::span<char> buffer) { reader::read(m_read_offset, buffer); }

	uint64_t tellg() { return m_read_offset; }

	virtual uint64_t size() = 0;

	virtual read_style get_read_style() const override { return read_style::sequential_only; }

	protected:
	virtual size_t raw_read_some(gsl::span<char> buffer) = 0;
	uint64_t m_read_offset{};
};

using unique_sequential_reader = std::unique_ptr<sequential_reader>;
} // namespace io_utility
