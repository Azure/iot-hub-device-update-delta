/**
 * @file chain_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <memory>

#include "reader.h"

namespace archive_diff::io::sequential
{
class chain_reader : public reader
{
	public:
	chain_reader(std::vector<std::unique_ptr<reader>> &&readers) : m_readers(std::move(readers))
	{
		m_size = 0;

		for (auto &reader : m_readers)
		{
			m_size += reader->size();
		}
	}

	virtual void skip(uint64_t to_skip)
	{
		auto remaining_to_skip = to_skip;
		while (remaining_to_skip > 0)
		{
			// We tried to skip more than we could process
			if (m_current_reader_index >= m_readers.size())
			{
				throw errors::user_exception(
					errors::error_code::io_chain_reader_skipping_too_much,
					"chain_reader::skip: Trying to skip more than is available in total.");
			}

			auto &current_reader = m_readers[m_current_reader_index];

			auto remaining_in_reader = current_reader->size() - m_offset_in_current_reader;

			// just move to next reader, we're done here.
			if (remaining_in_reader <= remaining_to_skip)
			{
				m_read_offset += remaining_in_reader;
				m_offset_in_current_reader = 0;
				m_current_reader_index++;
				remaining_to_skip -= remaining_in_reader;
				continue;
			}

			// this is the last reader we will be processing, just call skip on it.

			m_read_offset += remaining_to_skip;
			m_offset_in_current_reader += remaining_to_skip;
			current_reader->skip(remaining_to_skip);
			remaining_to_skip = 0;

			if (m_offset_in_current_reader != current_reader->tellg())
			{
				throw errors::user_exception(
					errors::error_code::io_chain_offset_mismatch,
					"chain_reader::skip: Offset in current reader doesn't match with expected value.");
			}
		}
	}

	virtual size_t read_some(std::span<char> buffer)
	{
		size_t total_read{};

		auto remaining_to_read = buffer.size();

		while (remaining_to_read > 0)
		{
			if (m_current_reader_index >= m_readers.size())
			{
				break;
			}

			auto &current_reader = m_readers[m_current_reader_index];

			std::span<char> reader_buffer{buffer.data() + total_read, buffer.size() - total_read};
			auto actual_read = current_reader->read_some(reader_buffer);

			if (actual_read == 0)
			{
				// shouldn't be possible
				std::string msg = "chain_reader::read_some: Couldn't read any data. remaining_to_read: "
				                + std::to_string(remaining_to_read);
				throw errors::user_exception(errors::error_code::io_chain_read_too_much, msg);
			}

			if (actual_read > remaining_to_read)
			{
				// shouldn't be possible
				std::string msg =
					"chain_reader::read_some: More read than remaining. actual_read: " + std::to_string(actual_read)
					+ ", remaining_to_read: " + std::to_string(remaining_to_read);
				throw errors::user_exception(errors::error_code::io_chain_read_too_much, msg);
			}

			total_read += actual_read;
			m_read_offset += actual_read;
			m_offset_in_current_reader += actual_read;
			remaining_to_read -= actual_read;

			if (current_reader->tellg() > current_reader->size())
			{
				std::string msg =
					"chain_reader::read_some: tellg() > size(). tellg: " + std::to_string(current_reader->tellg())
					+ ", size(): " + std::to_string(current_reader->size());
				throw errors::user_exception(errors::error_code::io_chain_bad_offset, msg);
			}

			// just move to next reader, we're done here.
			if (current_reader->tellg() == current_reader->size())
			{
				m_offset_in_current_reader = 0;
				m_current_reader_index++;
			}
		}

		return total_read;
	}

	virtual uint64_t tellg() const { return m_read_offset; }

	virtual uint64_t size() const { return m_size; }

	private:
	std::vector<std::unique_ptr<reader>> m_readers;
	uint64_t m_read_offset{};
	uint64_t m_offset_in_current_reader{};
	size_t m_current_reader_index{};
	uint64_t m_size{};
};
} // namespace archive_diff::io::sequential