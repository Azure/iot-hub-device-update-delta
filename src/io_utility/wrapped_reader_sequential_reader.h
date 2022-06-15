/**
 * @file wrapped_reader_sequential_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "sequential_reader.h"

namespace io_utility
{
class wrapped_reader_sequential_reader : public sequential_reader
{
	public:
	wrapped_reader_sequential_reader(reader *reader) : m_wrapped_reader(reader) {}

	virtual uint64_t size() override { return m_wrapped_reader->size(); }

	virtual void skip(uint64_t to_skip) override
	{
		if (m_wrapped_reader->get_read_style() == read_style::random_access)
		{
			m_read_offset += to_skip;
			return;
		}

		sequential_reader::skip(to_skip);
	}

	protected:
	virtual size_t raw_read_some(gsl::span<char> buffer) override
	{
		auto actual_read = m_wrapped_reader->read_some(m_read_offset, buffer);
		return actual_read;
	}

	private:
	reader *m_wrapped_reader{};
};
} // namespace io_utility