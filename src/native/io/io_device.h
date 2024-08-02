/**
 * @file io_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <span>

#include <errors/user_exception.h>

namespace archive_diff::io
{
class io_device
{
	public:
	virtual ~io_device() = default;

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) = 0;

	virtual uint64_t size() const = 0;
};

using unique_io_device = std::unique_ptr<io_device>;
using shared_io_device = std::shared_ptr<io_device>;
} // namespace archive_diff::io
