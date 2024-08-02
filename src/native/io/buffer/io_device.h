/**
 * @file io_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>
#include <cstring>

#include <span>

#include <io/io_device.h>
#include <io/reader.h>

namespace archive_diff::io::buffer
{
// This reader stores its data using an internal vector. We use capacity() or size()
// depending on the value of size_is_buffer_size passed in constructor.
// We support either to be compatible with io::buffer::writer which must use
// size(), because increasing capacity with reserve() doesn't maintain any existing
// data and we get a resulting corruption.
class io_device : public io::io_device
{
	public:
	enum class size_kind
	{
		vector_capacity,
		vector_size
	};

	virtual ~io_device() = default;

	io_device(std::shared_ptr<std::vector<char>> &stored_blob, size_kind kind) :
		m_buffer(stored_blob), m_size_kind(kind)
	{}

	static io::reader make_reader(std::shared_ptr<std::vector<char>> stored_blob, io_device::size_kind size_kind)
	{
		io::shared_io_device device = std::make_shared<io_device>(stored_blob, size_kind);
		return io::reader{device};
	}

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override
	{
		if (size() < offset)
		{
			std::string msg = "Attempting to read at offset " + std::to_string(offset)
			                + " but capacity is: " + std::to_string(size());
			throw errors::user_exception(errors::error_code::io_stored_blob_reader_read_offset, msg);
		}

		auto available = size() - offset;
		auto to_read   = static_cast<size_t>(std::min<uint64_t>(available, buffer.size()));

		std::memcpy(buffer.data(), m_buffer->data() + offset, to_read);

		return to_read;
	}

	virtual uint64_t size() const override
	{
		return (m_size_kind == size_kind::vector_size) ? m_buffer->size() : m_buffer->capacity();
	}

	private:
	std::shared_ptr<std::vector<char>> m_buffer;
	size_kind m_size_kind{};
};
} // namespace archive_diff::io::buffer