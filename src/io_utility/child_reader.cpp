/**
 * @file child_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <gsl/span>

#include "child_reader.h"
#include "user_exception.h"

#include "adu_log.h"

io_utility::child_reader::child_reader(reader *reader) : m_parent_reader(reader)
{
	if (m_parent_reader == nullptr)
	{
		throw error_utility::user_exception(error_utility::error_code::io_child_reader_parent_is_null);
	}
}

io_utility::child_reader::child_reader(reader *reader, uint64_t base_offset, uint64_t length) :
	m_parent_reader(reader), m_base_offset(base_offset), m_length(length)
{
	ADU_LOG("child_reader::child_reader(): base_offset=%llu, length=%llu", base_offset, length);

	if (m_parent_reader == nullptr)
	{
		throw error_utility::user_exception(error_utility::error_code::io_child_reader_parent_is_null);
	}

	auto end_offset = m_base_offset + m_length.value();

	if (end_offset > reader->size())
	{
		std::string msg = "New child_reader length would exceed end offset of parent reader. parent.size(): "
		                + std::to_string(reader->size()) + " base_offset: " + std::to_string(base_offset)
		                + " length: " + std::to_string(length) + " end_offset: " + std::to_string(end_offset);
		throw error_utility::user_exception(error_utility::error_code::io_child_reader_out_of_bounds, msg);
	}
}

io_utility::child_reader::child_reader(
	reader *reader, std::optional<uint64_t> base_offset, std::optional<uint64_t> length) :
	m_parent_reader(reader),
	m_length(length)
{
	if (base_offset.has_value())
	{
		m_base_offset = base_offset.value();
	}
}

uint64_t io_utility::child_reader::size()
{
	if (m_length.has_value())
	{
		return m_length.value();
	}

	return m_parent_reader->size();
}

size_t io_utility::child_reader::read_some(uint64_t offset, gsl::span<char> buffer)
{
	auto read_offset = offset + m_base_offset;

	ADU_LOG("child_reader::readsome(): offset=%llu, length=%zu. read_offset=%llu", offset, buffer.size(), read_offset);

	if (m_length.has_value())
	{
		auto max_length = m_length.value() - offset;
		auto to_read    = std::min(buffer.size(), static_cast<size_t>(max_length));
		buffer          = gsl::span<char>(buffer.data(), to_read);
	}

	auto actual_read = m_parent_reader->read_some(read_offset, buffer);

	return actual_read;
}

io_utility::reader::read_style io_utility::child_reader::get_read_style() const
{
	return m_parent_reader->get_read_style();
}
