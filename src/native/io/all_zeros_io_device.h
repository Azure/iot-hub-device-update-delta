/**
 * @file all_zeros_io_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/io_device.h>
#include <io/reader.h>

namespace archive_diff::io
{
class all_zeros_io_device : public io::io_device
{
	public:
	all_zeros_io_device(uint64_t length) : m_length(length) {}

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override;
	virtual uint64_t size() const override { return m_length; }

	static io::reader make_reader(uint64_t length)
	{
		std::shared_ptr<io::io_device> device = std::make_shared<all_zeros_io_device>(length);
		return io::reader{device};
	}

	private:
	uint64_t m_length{};
};
} // namespace archive_diff::io
