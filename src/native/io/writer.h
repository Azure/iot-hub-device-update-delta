/**
 * @file writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <string_view>

namespace archive_diff::io
{
class writer
{
	public:
	virtual ~writer() = default;

	virtual void write(uint64_t offset, std::string_view buffer) = 0;

	void write_uint8_t(uint64_t offset, uint8_t value);
	void write_uint16_t(uint64_t offset, uint16_t value);
	void write_uint32_t(uint64_t offset, uint32_t value);
	void write_uint64_t(uint64_t offset, uint64_t value);

	virtual void flush()                                         = 0;
	virtual uint64_t size() const                                = 0;
};

using unique_writer = std::unique_ptr<writer>;
using shared_writer = std::shared_ptr<writer>;
} // namespace archive_diff::io