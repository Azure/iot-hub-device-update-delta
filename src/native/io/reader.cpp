/**
 * @file reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <limits>
#include <string>

#include <span>

#include "reader.h"

#include <errors/adu_log.h>
#include <errors/user_exception.h>

namespace archive_diff::io
{
reader reader::simple_reader_impl::slice(uint64_t offset, uint64_t length) const
{
	std::shared_ptr<reader_impl> impl =
		std::make_shared<simple_reader_impl>(io_device_view(m_view, offset, length));
	return reader(impl);
}

std::vector<reader> reader::simple_reader_impl::unchain() const
{
	std::shared_ptr<reader_impl> impl = std::make_shared<simple_reader_impl>(io_device_view(m_view));
	return std::vector<reader>({reader{impl}});
}

void reader::read_all(std::vector<char> &data) const
{
	if (size() > std::numeric_limits<std::span<char>::size_type>::max())
	{
		std::string msg =
			"reader::read_all(): Attempting to read all data, but size() is larger than numeric limit."
			" reader size: "
			+ std::to_string(size())
			+ ", size_type::max(): " + std::to_string(std::numeric_limits<std::span<char>::size_type>::max());

		throw errors::user_exception(errors::error_code::io_zstd_dictionary_too_large, msg);
	}

	auto data_size = static_cast<std::span<char>::size_type>(size());
	data.resize(data_size);
	read(0, std::span<char>{data.data(), data_size});
}

reader reader::chained_reader_impl::slice(uint64_t offset, uint64_t length) const
{
	std::vector<reader> readers;

	auto lower_bound = std::lower_bound(m_offsets.begin(), m_offsets.end(), offset);

	if (lower_bound < m_offsets.begin())
	{
		std::string msg = "reader::chained_reader_impl::slice(): lower_bound too low. lower_bound: "
		                + std::to_string(*lower_bound) + ", m_offset.begin(): " + std::to_string(*m_offsets.begin());

		throw errors::user_exception(errors::error_code::io_reader_slice_bound_error, msg);
	}

	auto index       = static_cast<size_t>(lower_bound - m_offsets.begin());

	if (index != 0)
	{
		// This is past the last offset value, try last reader.
		if (index >= m_offsets.size())
		{
			index = m_offsets.size() - 1;
		}
		else if (offset < m_offsets[index])
		{
			index--;
		}
	}

	uint64_t offset_in_reader = offset - m_offsets[index];

	auto remaining      = length;
	while ((remaining > 0) && (index < m_readers.size()))
	{
		auto &reader               = m_readers[index];
		auto reader_bytes_consumed = std::min(reader.size() - offset_in_reader, remaining);

		// If we're not taking the entire reader, then take a slice
		if (offset_in_reader || (reader_bytes_consumed < reader.size()))
		{
			readers.push_back(reader.slice(offset_in_reader, reader_bytes_consumed));
		}
		else
		{
			readers.push_back(reader);
		}

		remaining -= reader_bytes_consumed;
		offset_in_reader = 0;
		index++;
	}
	return reader::chain(readers);
}

reader::chained_reader_impl::chained_reader_impl(const chained_reader_impl &chained, const reader_impl *other) :
	chained_reader_impl(chained)
{
	auto other_readers = other->unchain();

	for (auto &reader : other_readers)
	{
		m_offsets.push_back(m_length);
		m_readers.push_back(reader);
		m_length += reader.size();
	}
}

// reader reader::chained_reader_impl::chain(reader &other) const
//{
//	std::shared_ptr<reader_impl> impl = std::make_shared<chained_reader_impl>(*this, other.m_impl.get());
//	return reader{impl};
// }

size_t reader::chained_reader_impl::read_some(uint64_t offset, std::span<char> buffer) const
{
	if ((buffer.size() == 0) || (m_length == 0))
	{
		return 0;
	}

	auto lower_bound = std::lower_bound(m_offsets.begin(), m_offsets.end(), offset);

	if (lower_bound < m_offsets.begin())
	{
		std::string msg = "reader::chained_reader_impl::slice(): lower_bound too low. lower_bound: "
		                + std::to_string(*lower_bound) + ", m_offset.begin(): " + std::to_string(*m_offsets.begin());

		throw errors::user_exception(errors::error_code::io_reader_slice_bound_error, msg);
	}

	auto index = static_cast<size_t>(lower_bound - m_offsets.begin());

	if (index != 0)
	{
		// This is past the last offset value, try last reader.
		if (index >= m_offsets.size())
		{
			index = m_offsets.size() - 1;
		}
		else if (offset < m_offsets[index])
		{
			index--;
		}
	}

	uint64_t offset_in_reader = offset - m_offsets[index];

	auto remaining      = buffer.size();
	uint64_t total_read = 0;
	while (remaining > 0 && index < m_readers.size())
	{
		auto &reader = m_readers[index];
		std::span<char> remaining_buffer{buffer.data() + total_read, remaining};
		auto actual_read = reader.read_some(offset_in_reader, remaining_buffer);

		if (actual_read == 0)
		{
			break;
		}

		remaining -= actual_read;
		total_read += actual_read;
		index++;
		offset_in_reader = 0;
	}

	return total_read;
}

} // namespace archive_diff::io