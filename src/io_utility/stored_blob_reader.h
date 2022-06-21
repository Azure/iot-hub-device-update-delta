/**
 * @file stored_blob_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>
#include <cstring>

#include <gsl/span>

#include "reader.h"

namespace io_utility
{
class stored_blob_reader : public reader
{
	public:
	virtual ~stored_blob_reader() = default;

	stored_blob_reader(std::shared_ptr<std::vector<char>> stored_blob, uint64_t length) :
		m_stored_blob(stored_blob), m_length(length)
	{}

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer)
	{
		if (m_length < offset)
		{
			std::string msg = "Attempting to read at offset " + std::to_string(offset)
			                + " but length is: " + std::to_string(m_length);
			throw error_utility::user_exception(error_utility::error_code::io_stored_blob_reader_read_offset, msg);
		}

		auto available = m_length - offset;
		auto to_read   = std::min<size_t>(available, static_cast<unsigned long long>(buffer.size()));

		std::memcpy(buffer.data(), m_stored_blob->data() + offset, to_read);

		return to_read;
	}

	virtual uint64_t size()
	{
		// TODO: Use something besides capacity
		return m_stored_blob->capacity();
	}

	virtual read_style get_read_style() const override { return read_style::random_access; }

	private:
	std::shared_ptr<std::vector<char>> m_stored_blob;
	uint64_t m_offset{};
	uint64_t m_length{};
};
} // namespace io_utility