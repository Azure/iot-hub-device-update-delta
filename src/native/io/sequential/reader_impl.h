/**
 * @file reader_impl.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"

namespace archive_diff::io::sequential
{
class reader_impl : public reader
{
	public:
	virtual void skip(uint64_t to_skip) override;

	virtual size_t read_some(std::span<char> buffer)
	{
		auto actual_read = raw_read_some(buffer);
		m_read_offset += actual_read;
		return actual_read;
	}

	virtual uint64_t tellg() const override { return m_read_offset; }

	protected:
	virtual size_t raw_read_some(std::span<char> buffer) = 0;
	uint64_t m_read_offset{};
};
} // namespace archive_diff::io::sequential