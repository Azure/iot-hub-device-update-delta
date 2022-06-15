/**
 * @file chained_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "chained_reader.h"

io_utility::chained_reader::chained_reader(gsl::span<reader *> readers)
{
	for (auto reader : readers)
	{
		m_readers.push_back(reader);
	}

	process_readers();
}

io_utility::chained_reader::chained_reader(std::vector<unique_reader> &&readers) : m_readers_storage(std::move(readers))
{
	for (auto &reader : m_readers_storage)
	{
		m_readers.push_back(reader.get());
	}

	process_readers();
}

void io_utility::chained_reader::process_readers()
{
	uint64_t size    = 0;
	read_style style = reader::read_style::random_access;
	for (const auto &reader : m_readers)
	{
		size += reader->size();
		if (reader->get_read_style() == read_style::sequential_only)
		{
			style = read_style::sequential_only;
		}
	}

	m_size       = size;
	m_read_style = style;
}

io_utility::reader *io_utility::chained_reader::get_reader_for_offset(uint64_t offset)
{
	size_t i_reader = 0;
	if (m_current_reader_index.has_value())
	{
		i_reader    = m_current_reader_index.value();
		auto reader = m_readers[i_reader];

		if ((offset >= m_start_of_current_reader) && (offset < m_end_of_current_reader))
		{
			return reader;
		}

		if (offset < m_start_of_current_reader)
		{
			i_reader                  = 0;
			m_start_of_current_reader = 0;
			m_end_of_current_reader   = 0;
		}
		else
		{
			m_start_of_current_reader = m_end_of_current_reader;
			i_reader++;
		}
	}

	for (; i_reader < m_readers.size(); i_reader++, m_start_of_current_reader = m_end_of_current_reader)
	{
		auto &reader = m_readers[i_reader];

		m_end_of_current_reader += reader->size();

		if (offset >= m_end_of_current_reader)
		{
			continue;
		}

		m_current_reader_index = i_reader;
		return reader;
	}

	return nullptr;
}

size_t io_utility::chained_reader::read_some(uint64_t offset, gsl::span<char> buffer)
{
	auto remaining    = buffer.size();
	auto read_offset  = offset;
	auto write_offset = 0ull;

	io_utility::reader *reader;
	do
	{
		reader = get_reader_for_offset(read_offset);

		auto to_read = std::min<size_t>(m_end_of_current_reader - read_offset, remaining);
		if (to_read > remaining)
		{
			std::string msg = "Before read: Not enough data to read from chained reader. Remaining: "
			                + std::to_string(to_read) + ", to read: " + std::to_string(to_read);
			throw error_utility::user_exception(error_utility::error_code::io_chained_reader_not_enough_data, msg);
		}

		auto offset_in_reader = read_offset - m_start_of_current_reader;

		if (offset_in_reader >= reader->size())
		{
			std::string msg = "Offset in reader past end. Offset: " + std::to_string(offset_in_reader)
			                + ", Size: " + std::to_string(reader->size());
			throw error_utility::user_exception(error_utility::error_code::io_chained_reader_not_enough_data, msg);
		}

		// printf(
		// "[%p][%llu] = [%p]: Reading %llu bytes at %llu offset.\n",
		// this,
		// m_current_reader_index.value(),
		// reader,
		// to_read,
		// offset);

		reader->read(offset_in_reader, gsl::span{buffer.data() + write_offset, to_read});

		read_offset += to_read;
		write_offset += to_read;

		if (to_read > remaining)
		{
			std::string msg = "After read: Not enough data to read from chained reader. Remaining: "
			                + std::to_string(to_read) + ", to read: " + std::to_string(to_read);
			throw error_utility::user_exception(error_utility::error_code::io_chained_reader_not_enough_data, msg);
		}

		remaining -= to_read;
	}
	while (remaining > 0 && (reader != nullptr));

	return buffer.size() - remaining;
}

io_utility::reader::read_style io_utility::chained_reader::get_read_style() const { return m_read_style; }

uint64_t io_utility::chained_reader::size() { return m_size; }