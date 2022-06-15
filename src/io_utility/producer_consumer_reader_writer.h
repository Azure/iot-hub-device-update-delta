/**
 * @file producer_consumer_reader_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <exception>
#include <mutex>
#include <condition_variable>

#include "sequential_reader.h"
#include "sequential_writer.h"

namespace io_utility
{
class producer_consumer_reader_writer : public sequential_reader, public sequential_writer
{
	public:
	class writer_cancelled_exception : public std::exception
	{};

	const size_t producer_consumer_reader_writer_capacity = 64 * 1024;

	public:
	producer_consumer_reader_writer(uint64_t expected_read_size) : m_expected_read_size(expected_read_size)
	{
		m_buffer.reserve(producer_consumer_reader_writer_capacity);
	}

	void notify_writer() { m_waiting_writer_cv.notify_all(); }

	/* writer methods */
	virtual void write(uint64_t offset, std::string_view buffer) override
	{
		if (offset != m_written)
		{
			std::string msg = "Attempting to write to offset " + std::to_string(offset)
			                + ", but does not match m_written " + std::to_string(m_written);
			throw error_utility::user_exception(
				error_utility::error_code::io_producer_consumer_reader_writer_invalid_offset, msg);
		}

		write(buffer);
	}

	/* sequential_writer methods */

	virtual uint64_t tellp(void) override { return m_written; }

	virtual void write(std::string_view buffer) override { write_impl(buffer); }

	protected:
	virtual void write_impl(std::string_view buffer) override
	{
		auto remaining     = buffer.length();
		size_t read_offset = 0;

		while (true)
		{
			if (m_cancelled)
			{
				throw writer_cancelled_exception();
			}

			// we won't get anything more from the reader here, this is a failure
			if (m_read >= m_expected_read_size)
			{
				std::string msg =
					"Attempting to write when we've already read expected amount. m_read: " + std::to_string(m_read)
					+ ", m_expected_read_size: " + std::to_string(m_expected_read_size);
				throw error_utility::user_exception(
					error_utility::error_code::io_producer_consumer_reader_writer_writing_when_done, msg);
			}

			{
				std::lock_guard<std::mutex> lock_guard(m_buffer_mutex);

				shift_buffer();

				auto available = get_available_write();

				auto to_write = std::min(remaining, available);
				memcpy(m_buffer.data() + m_end_offset, buffer.data() + read_offset, to_write);
				m_end_offset += to_write;

				remaining -= to_write;
				m_written += to_write;
				read_offset += to_write;
			}

			// we wrote something, signal any waiting readers
			m_waiting_reader_cv.notify_all();

			if (remaining == 0)
			{
				return;
			}

			// wait for a signal
			std::unique_lock<std::mutex> lock(m_waiting_writer_mutex);
			m_waiting_writer_cv.wait(
				lock, [&] { return m_start_offset != 0 || m_cancelled || m_read == m_expected_read_size; });
		}
	}

	virtual void flush(void) override { shift_buffer(); }

	/* sequential_reader methods */
	protected:
	size_t get_available_read() { return m_end_offset - m_start_offset; }
	size_t get_available_write() { return m_buffer.capacity() - m_end_offset; }

	virtual size_t raw_read_some(gsl::span<char> buffer) override
	{
		auto available = get_available_read();

		while (available == 0)
		{
			if (m_read == m_expected_read_size)
			{
				return 0;
			}

			// wait for a signal
			std::unique_lock<std::mutex> lock(m_waiting_reader_mutex);
			m_waiting_reader_cv.wait(lock, [&] { return m_start_offset != m_end_offset; });

			available = get_available_read();
		}

		auto to_read = std::min<size_t>(available, buffer.size());

		{
			std::lock_guard<std::mutex> lock(m_buffer_mutex);

			auto max_to_read = m_expected_read_size - m_read;
			// we shouldn't read more than we think is available
			if (max_to_read < to_read)
			{
				std::string msg = "More data is available than we expect. m_read: " + std::to_string(m_read)
				                + ", max_to_read: " + std::to_string(max_to_read);
				throw error_utility::user_exception(
					error_utility::error_code::io_producer_consumer_reader_writer_reading_too_much_available, msg);
			}

			memcpy(buffer.data(), m_buffer.data() + m_start_offset, to_read);

			m_start_offset += to_read;
			m_read += to_read;
		}

		m_waiting_writer_cv.notify_all();
		return to_read;
	}

	public:
	virtual uint64_t size(void) override { return m_expected_read_size; }

	private:
	void shift_buffer()
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

	bool m_cancelled{false};

	uint64_t m_written{};
	uint64_t m_read{};
	uint64_t m_expected_read_size{};
	std::vector<char> m_buffer;
	size_t m_start_offset{};
	size_t m_end_offset{};

	std::mutex m_buffer_mutex;

	std::mutex m_waiting_writer_mutex;
	std::condition_variable m_waiting_writer_cv;

	std::mutex m_waiting_reader_mutex;
	std::condition_variable m_waiting_reader_cv;
};
} // namespace io_utility