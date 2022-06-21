/**
 * @file wrapped_readerwriter.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "readerwriter.h"

namespace io_utility
{
class wrapped_readerwriter : public io_utility::readerwriter
{
	public:
	wrapped_readerwriter(io_utility::readerwriter *wrapped) : m_wrapped(wrapped) {}

	virtual ~wrapped_readerwriter() = default;

	// reader methods
	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer) override
	{
		return m_wrapped->read_some(offset, buffer);
	}
	virtual read_style get_read_style() const override { return m_wrapped->get_read_style(); }
	virtual uint64_t size() override { return m_wrapped->size(); }

	// writer methods
	virtual void write(uint64_t offset, std::string_view buffer) override { m_wrapped->write(offset, buffer); }
	virtual void flush() override { m_wrapped->flush(); }

	private:
	io_utility::readerwriter *m_wrapped{nullptr};
};
} // namespace io_utility