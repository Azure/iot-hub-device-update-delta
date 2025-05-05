/**
 * @file writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/reader.h>
#include <io/writer.h>

#include "reader.h"

namespace archive_diff::io::sequential
{
// completely abstract version can be subclasses and have
// all method implemented. For instance can be alternatively
// composed of different other objects that already track
// offset.
class writer
{
	public:
	virtual ~writer() = default;

	virtual void write(const io::reader &reader);
	virtual void write(io::sequential::reader &reader);
	virtual void write(std::string_view buffer) = 0;
	virtual void flush()                        = 0;

	void write_value(const std::string &value)
	{
		uint64_t size = value.size();
		write_uint64_t(size);
		write(std::string_view{value.data(), value.size()});
	}

	void write_uint8_t(uint8_t value);
	void write_uint16_t(uint16_t value);
	void write_uint32_t(uint32_t value);
	void write_uint64_t(uint64_t value);

	template <typename T>
	void write_value(const T *value)
	{
		std::string_view buffer{reinterpret_cast<const char *>(value), sizeof(T)};
		write(buffer);
	}

	void write_text(const std::string &value) { write(std::string_view{value.data(), value.size()}); }

	template <typename T>
	void write(const T *value)
	{
		std::string_view buffer{reinterpret_cast<const char *>(value), sizeof(T)};
		write(buffer);
	}

	virtual void write(std::span<char> buffer)
	{
		std::string_view buffer_as_view{buffer.data(), buffer.size()};
		write(buffer_as_view);
	}

	virtual uint64_t tellp() = 0;
};
} // namespace archive_diff::io::sequential