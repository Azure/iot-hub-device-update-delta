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
	virtual void flush()                                         = 0;
	virtual uint64_t size() const                                = 0;
};

using unique_writer = std::unique_ptr<writer>;
using shared_writer = std::shared_ptr<writer>;
} // namespace archive_diff::io