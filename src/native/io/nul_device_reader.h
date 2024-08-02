/**
 * @file nul_device_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include "io_device.h"

namespace archive_diff::io
{
// This device stores no data and all reads yield no information.
class nul_io_device : public io_device
{
	public:
	virtual ~nul_io_device() = default;
	virtual size_t read_some(uint64_t, std::span<char>) override { return 0; }
	virtual uint64_t size() const override { return 0; }
};
} // namespace archive_diff::io
