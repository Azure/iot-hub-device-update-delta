/**
 * @file basic_reader_wrapper.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include "reader_impl.h"

namespace archive_diff::io::sequential
{
class basic_reader_wrapper : public reader_impl
{
	public:
	basic_reader_wrapper(const io::reader &reader) : m_wrapped_reader(reader) {}

	virtual uint64_t size() const override { return m_wrapped_reader.size(); }

	virtual void skip(uint64_t to_skip) override
	{
		m_read_offset += to_skip;
		return;
	}

	static std::shared_ptr<archive_diff::io::sequential::reader> make_shared(io::reader &reader)
	{
		return std::make_shared<basic_reader_wrapper>(reader);
	}

	static std::shared_ptr<archive_diff::io::sequential::reader> make_unique(io::reader &reader)
	{
		return std::make_unique<basic_reader_wrapper>(reader);
	}

	protected:
	virtual size_t raw_read_some(std::span<char> buffer) override
	{
		return m_wrapped_reader.read_some(m_read_offset, buffer);
	}

	private:
	const io::reader m_wrapped_reader;
};
} // namespace archive_diff::io::sequential