/**
 * @file reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "gsl/span"
#include <memory>

#include "user_exception.h"

namespace io_utility
{
class reader
{
	public:
	virtual ~reader() = default;

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer) = 0;
	void read(uint64_t offset, gsl::span<char> buffer)
	{
		size_t actual = read_some(offset, buffer);
		if (actual != buffer.size())
		{
			std::string msg = "Wanted to read " + std::to_string(buffer.size()) + " bytes, but could only read "
			                + std::to_string(actual) + " bytes.";
			throw error_utility::user_exception(error_utility::error_code::io_reader_read_failure, msg);
		}
	}

	enum class read_style
	{
		none,
		sequential_only,
		random_access,
	};

	virtual read_style get_read_style() const = 0;

	virtual uint64_t size() = 0;
};

using unique_reader = std::unique_ptr<reader>;
} // namespace io_utility
