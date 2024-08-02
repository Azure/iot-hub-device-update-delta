/**
 * @file writer_impl.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "writer.h"

namespace archive_diff::io::sequential
{
// basic implementation with its own offset
class writer_impl : public writer
{
	public:
	virtual ~writer_impl() = default;

	virtual void write(const io::reader &reader) override { writer::write(reader); }
	virtual void write(io::sequential::reader &reader) override { writer::write(reader); }
	virtual void write(std::span<char> buffer) override { writer::write(buffer); }
	virtual void write(std::string_view buffer) override;

	virtual uint64_t tellp() override;

	protected:
	virtual void write_impl(std::string_view buffer) = 0;
	uint64_t m_offset{};
};
} // namespace archive_diff::io::sequential