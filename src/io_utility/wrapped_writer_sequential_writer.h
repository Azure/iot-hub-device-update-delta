/**
 * @file wrapped_writer_sequential_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "sequential_writer.h"

namespace io_utility
{
class wrapped_writer_sequential_writer : public sequential_writer_impl
{
	public:
	wrapped_writer_sequential_writer(writer *writer) : m_writer(writer) {}

	virtual void flush() { m_writer->flush(); }

	protected:
	virtual void write_impl(std::string_view buffer) { m_writer->write(m_offset, buffer); }

	private:
	writer *m_writer{};
};
} // namespace io_utility