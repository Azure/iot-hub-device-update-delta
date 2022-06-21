/**
 * @file sequential_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "writer.h"
#include "sequential_reader.h"

namespace io_utility
{
// completely abstract version can be subclasses and have
// all method simplemented. For instance can be alternatively
// composed of different other objects that already track
// offset.
class sequential_writer : public writer
{
	public:
	virtual ~sequential_writer() = default;

	virtual void write(io_utility::reader *reader);
	virtual void write(io_utility::sequential_reader *reader);
	virtual void write(std::string_view buffer) = 0;

	template <typename T>
	void write_value(const T &value)
	{
		std::string_view buffer{&value, sizeof(T)};
		write(buffer);
	}

	template <typename T>
	void write_value(T *value)
	{
		std::string_view buffer{value, sizeof(T)};
		write(buffer);
	}

	virtual uint64_t tellp() = 0;

	protected:
	virtual void write_impl(std::string_view buffer) = 0;
};

// basic implementation with its own offset
class sequential_writer_impl : public sequential_writer
{
	public:
	virtual ~sequential_writer_impl() = default;

	virtual void write(uint64_t offset, std::string_view buffer);
	virtual void write(io_utility::reader *reader) { sequential_writer::write(reader); }
	virtual void write(io_utility::sequential_reader *reader) { sequential_writer::write(reader); }
	virtual void write(std::string_view buffer);
	virtual uint64_t tellp();

	protected:
	virtual void write_impl(std::string_view buffer) = 0;
	uint64_t m_offset{};
};
} // namespace io_utility