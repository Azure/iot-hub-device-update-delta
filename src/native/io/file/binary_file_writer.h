/**
 * @file binary_file_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/writer.h>
#include "file.h"

namespace archive_diff::io::file
{
class binary_file_writer : public writer
{
	public:
	binary_file_writer(const std::string &path);

	virtual void flush() override;
	virtual void write(uint64_t offset, std::string_view buffer) override;
	virtual uint64_t size() const override;

	virtual ~binary_file_writer() = default;

	private:
	mutable file m_File;
};
} // namespace archive_diff::io::file