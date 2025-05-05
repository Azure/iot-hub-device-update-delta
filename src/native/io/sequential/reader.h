/**
 * @file reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/reader.h>

namespace archive_diff::io::sequential
{
class reader
{
	public:
	virtual ~reader() = default;

	virtual void skip(uint64_t to_skip)              = 0;
	virtual size_t read_some(std::span<char> buffer) = 0;
	virtual uint64_t tellg() const                   = 0;
	virtual uint64_t size() const                    = 0;

	void read(std::span<char> buffer);
	void read(std::string *value);

	void read_uint8_t(uint8_t *value);
	void read_uint16_t(uint16_t *value);
	void read_uint32_t(uint32_t *value);
	void read_uint64_t(uint64_t *value);

	void read_all_remaining(std::vector<char> &buffer);

	uint64_t available() const { return size() - tellg(); }

	protected:
	void skip_by_reading(uint64_t to_skip);
};

using unique_reader = std::unique_ptr<reader>;
} // namespace archive_diff::io::sequential
