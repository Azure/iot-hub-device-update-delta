/**
 * @file buffer_io_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstring>

#include <io/io_device.h>
#include <string_view>

namespace archive_diff::io::test
{
// Not a safe device, it uses a string_view - it doesn't own the data.
class buffer_io_device : public io::io_device
{
	public:
	buffer_io_device(std::string_view buffer) : m_buffer(buffer) {}
	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override
	{
		auto available = static_cast<uint64_t>(m_buffer.size());
		if (offset > available)
		{
			return 0;
		}
		available -= offset;

		auto to_read = static_cast<size_t>(std::min<uint64_t>(buffer.size(), available));

		std::memcpy(buffer.data(), m_buffer.data() + offset, to_read);

		return to_read;
	}
	virtual uint64_t size() const override { return m_buffer.size(); }

	private:
	std::string_view m_buffer;
};
} // namespace archive_diff::io::test