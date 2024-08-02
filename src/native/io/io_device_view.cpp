/**
 * @file io_device_view.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "io_device_view.h"

namespace archive_diff::io
{
	io_device_view(const io_device_view &parent, uint64_t offset, uint64_t length) :
		m_device(parent.m_device), m_offset(parent.m_offset + offset), m_length(length)
	{
		auto new_end = m_offset + length;

		if (new_end > m_device->size())
		{
			std::string msg = "io_device_view::io_device_view: new_end: " + std::to_string(new_end)
		                    + ", m_device->size(): " + std::to_string(m_device->size());
			throw errors::user_exception(errors::error_code::io_device_new_end_past_size, msg);
		}
	}

	io_device_view slice(uint64_t offset, uint64_t length) const
	{
		return io_device_view(*this, offset, length);
	}

	uint64_t read_some(uint64_t offset, std::span<char> buffer) const
	{
		if (offset >= size())
		{
			return 0;
		}

		auto capacity = size() - offset;
		auto to_read  = std::min(buffer.size(), capacity);
		buffer        = std::span<char>(buffer.data(), to_read);

		auto effective_offset = m_offset + offset;
		auto actual_read      = m_device->read_some(effective_offset, buffer);

		return actual_read;
	}
};
} // namespace archive_diff::io