/**
 * @file io_device.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <io/io_device.h>
#include <io/reader.h>
#include <errors/adu_log.h>

#include "file.h"

namespace archive_diff::io::file
{
class io_device : public io::io_device
{
	public:
	io_device(const std::string &path);
	virtual ~io_device() {}

	static reader make_reader(const std::string &path);

	virtual size_t read_some(uint64_t offset, std::span<char> buffer) override;

	virtual uint64_t size() const override;

	private:
	mutable file m_File;
};
} // namespace archive_diff::io::file