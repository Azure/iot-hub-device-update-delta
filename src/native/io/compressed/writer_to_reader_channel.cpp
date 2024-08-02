/**
 * @file writer_to_reader_channel.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "writer_to_reader_channel.h"

#include <cstring>

namespace archive_diff::io::compressed
{
size_t writer_to_reader_channel::read_some(std::span<char> buffer)
{
	auto remaining_to_read = buffer.size();
	size_t total_read      = 0;

	while (remaining_to_read)
	{
		if (done_reading())
		{
			return total_read;
		}

		auto available = get_available_read();
		while (available == 0)
		{
			if (done_reading())
			{
				return total_read;
			}

			// wait for a signal
			wait_for_available_content();

			if (canceled())
			{
				return total_read;
			}

			available = get_available_read();
		}

		auto to_read = std::min<size_t>(available, remaining_to_read);

		{
			std::lock_guard<std::mutex> lock(m_buffer_mutex);
			auto max_to_read = get_max_available_read();
			// we shouldn't read more than we think is available
			if (max_to_read < to_read)
			{
				std::string msg = "More data is available than we expect. m_total_read: " + std::to_string(m_total_read)
				                + ", max_to_read: " + std::to_string(max_to_read);
				throw errors::user_exception(
					errors::error_code::io_producer_consumer_reader_writer_reading_too_much_available, msg);
			}
			std::memcpy(buffer.data() + total_read, m_buffer.data() + m_start_offset, to_read);

			m_start_offset += to_read;
			m_total_read += to_read;
		}

		notify_all_writers();

		remaining_to_read -= to_read;
		total_read += to_read;
	}

	return total_read;
}

void writer_to_reader_channel::write(std::string_view buffer)
{
	auto remaining     = buffer.length();
	size_t read_offset = 0;

	while (true)
	{
		// we won't get anything more from the reader here, this is a failure
		if (done_reading())
		{
			std::string msg = "Attempting to write when we've already read expected amount. m_total_read: "
			                + std::to_string(m_total_read)
			                + ", expected_total_read: " + std::to_string(m_expected_total_read);
			throw errors::user_exception(errors::error_code::io_producer_consumer_reader_writer_writing_when_done, msg);
		}

		{
			std::lock_guard<std::mutex> lock_guard(m_buffer_mutex);

			shift_buffer();

			auto available = get_available_write();
			auto to_write  = std::min<uint64_t>(remaining, available);

			memcpy(m_buffer.data() + m_end_offset, buffer.data() + read_offset, to_write);
			m_end_offset += to_write;

			remaining -= to_write;
			m_total_written += to_write;
			read_offset += to_write;
		}

		// we wrote something, signal any waiting readers
		notify_all_readers();

		if (remaining == 0)
		{
			return;
		}

		// wait for a signal
		wait_until_write_possible();

		if (canceled())
		{
			return;
		}
	}
}

void writer_to_reader_channel::shift_buffer()
{
	if (m_start_offset == 0)
	{
		return;
	}
	auto bytes = m_end_offset - m_start_offset;
	memcpy(m_buffer.data(), m_buffer.data() + m_start_offset, bytes);
	m_end_offset   = m_end_offset - m_start_offset;
	m_start_offset = 0;
}

void writer_to_reader_channel::wait_for_available_content()
{
	std::unique_lock<std::mutex> lock(m_buffer_mutex);
	m_waiting_reader_cv.wait(lock, [&] { return m_start_offset != m_end_offset || canceled(); });
}

void writer_to_reader_channel::wait_until_write_possible()
{
	std::unique_lock<std::mutex> lock(m_buffer_mutex);
	m_waiting_writer_cv.wait(lock, [&] { return !buffer_is_full() || done_reading() || canceled(); });
}
} // namespace archive_diff::io::compressed