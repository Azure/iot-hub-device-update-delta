/**
 * @file writer_to_reader_channel.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <exception>
#include <mutex>
#include <condition_variable>

#include <io/sequential/reader.h>
#include <io/sequential/writer.h>

namespace archive_diff::io::compressed
{
// This class is written to and allows a user to read the content written to the shared buffer.
class writer_to_reader_channel : public io::sequential::writer, public io::sequential::reader
{
	public:
	const size_t c_buffer_capacity = 64 * 1024;

	writer_to_reader_channel(uint64_t expected_total_read) : m_expected_total_read(expected_total_read)
	{
		m_buffer.reserve(c_buffer_capacity);
	}

	virtual ~writer_to_reader_channel() = default;

	virtual size_t read_some(std::span<char> buffer) override;
	virtual uint64_t tellg() const override { return m_total_read; }
	virtual void skip(uint64_t to_skip) { skip_by_reading(to_skip); }
	virtual uint64_t size() const { return m_expected_total_read; }

	virtual void write(std::string_view buffer) override;
	virtual uint64_t tellp() override { return m_total_written; }
	virtual void flush() override { shift_buffer(); }

	void notify_all_writers() { m_waiting_writer_cv.notify_all(); }
	void cancel()
	{
		m_canceled = true;
		notify_all_writers();
	}

	private:
	void shift_buffer();

	uint64_t get_max_available_read() const { return done_reading() ? 0 : m_expected_total_read - m_total_read; }

	uint64_t get_available_read() const { return m_end_offset - m_start_offset; }
	size_t get_available_write() const { return m_buffer.capacity() - m_end_offset; }

	void notify_all_readers() { m_waiting_reader_cv.notify_all(); }

	void wait_for_available_content();

	bool done_reading() const { return (m_total_read >= m_expected_total_read); }
	bool buffer_is_full() const { return (m_start_offset) == 0 && (m_end_offset == m_buffer.capacity()); }
	bool canceled() { return m_canceled; }

	void wait_until_write_possible();

	std::vector<char> m_buffer;
	size_t m_start_offset{};
	size_t m_end_offset{};

	uint64_t m_total_read{};
	uint64_t m_total_written{};

	uint64_t m_expected_total_read{};

	std::mutex m_buffer_mutex{};

	std::condition_variable m_waiting_writer_cv{};
	std::condition_variable m_waiting_reader_cv{};

	bool m_canceled{false};
};
} // namespace archive_diff::io::compressed