/**
 * @file writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>
#include <cstring>

#include <io/writer.h>

namespace archive_diff::io::buffer
{
// We must use size() + resize(), because increasing capacity with reserve()
// doesn't maintain any existing data and we get a resulting corruption.
class writer : public io::writer
{
	public:
	writer(std::shared_ptr<std::vector<char>> &buffer) : m_buffer(buffer) {}
	virtual ~writer() = default;

	virtual void write(uint64_t offset, std::string_view buffer)
	{
		auto minimum_length = offset + buffer.size();
		if (minimum_length > std::numeric_limits<size_t>::max())
		{
			throw std::exception();
		}

		if (m_buffer->size() < minimum_length)
		{
			m_buffer->resize(static_cast<size_t>(minimum_length));
		}

		std::memcpy(m_buffer->data() + offset, buffer.data(), buffer.size());
	}

	virtual void flush() override {}

	virtual uint64_t size() const override { return m_buffer->size(); }

	private:
	std::shared_ptr<std::vector<char>> m_buffer;
};

using unique_writer = std::unique_ptr<writer>;
} // namespace archive_diff::io::buffer