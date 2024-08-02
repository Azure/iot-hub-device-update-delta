/**
 * @file io_device_view.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include <optional>

#include "io_device.h"

namespace archive_diff::io
{
class io_device_view
{
	public:
	io_device_view(shared_io_device &device) : m_device(device), m_offset(0), m_length(std::nullopt)
	{
	}

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

	size_t read_some(uint64_t offset, std::span<char> buffer) const
	{
		if (offset >= size())
		{
			return 0;
		}

		auto capacity = size() - offset;
		auto to_read  = static_cast<size_t>(std::min<uint64_t>(buffer.size(), capacity));
		buffer        = std::span<char>(buffer.data(), to_read);

		auto effective_offset = m_offset + offset;
		auto actual_read = m_device->read_some(effective_offset, buffer);

		return actual_read;
	}

	uint64_t get_offset_in_device() const { return m_offset; }
	uint64_t size() const 
	{
		return m_length.value_or(m_device->size() - m_offset);
	}

	private:
	shared_io_device m_device;
	uint64_t m_offset;
	std::optional<uint64_t> m_length;
};
} // namespace archive_diff::io