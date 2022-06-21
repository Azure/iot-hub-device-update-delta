/**
 * @file stored_blob_reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>

#include "reader_factory.h"
#include "stored_blob_reader.h"

namespace io_utility
{
class stored_blob_reader_factory : public reader_factory
{
	public:
	virtual ~stored_blob_reader_factory() = default;

	stored_blob_reader_factory(reader *source_reader, uint64_t offset, uint64_t length) : m_length(length)
	{
		if (length > SIZE_MAX)
		{
			std::string msg = "stored_blob_reader_factory length larger than SIZE_MAX(" + std::to_string(SIZE_MAX)
			                + "). Length: " + std::to_string(length);
			throw error_utility::user_exception(
				error_utility::error_code::io_stored_blob_reader_factory_too_large, msg);
		}

		m_stored_blob = std::make_shared<std::vector<char>>();
		m_stored_blob->reserve(m_length);
		source_reader->read(offset, gsl::span<char>{m_stored_blob->data(), static_cast<size_t>(m_length)});
	}

	virtual std::unique_ptr<reader> create() override
	{
		return std::make_unique<stored_blob_reader>(m_stored_blob, m_length);
	}

	protected:
	std::shared_ptr<std::vector<char>> m_stored_blob;
	uint64_t m_length{};
};
} // namespace io_utility
