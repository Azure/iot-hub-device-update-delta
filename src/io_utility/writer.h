/**
 * @file writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include <gsl/span>
#include <string_view>

namespace io_utility
{
class writer
{
	public:
	virtual ~writer() = default;

	virtual void write(uint64_t offset, std::string_view buffer) = 0;
	virtual void flush()                                         = 0;
};

using unique_writer = std::unique_ptr<writer>;
} // namespace io_utility