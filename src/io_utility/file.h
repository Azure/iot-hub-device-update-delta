/**
 * @file file.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <stdio.h>
#include <memory>

#include <filesystem>
#include <gsl/span>

#include "error_codes.h"

namespace io_utility
{
class file
{
	public:
	enum class mode
	{
		read,
		write,
		read_write
	};

	file(const std::string &file, mode mode, error_utility::error_code error);
	file(FILE *fp) : m_fp(fp) {}

	size_t read_some(uint64_t offset, gsl::span<char> buffer);
	uint64_t size();
	void write(uint64_t offset, std::string_view buffer);
	void flush();

	struct FILE_Deleter
	{
		void operator()(FILE *fp)
		{
			if (fp != nullptr)
			{
				fclose(fp);
			}
		}
	};
	using unique_FILE = std::unique_ptr<FILE, FILE_Deleter>;

	private:
	uint64_t seek(int64_t offset, int origin);
	uint64_t tell();

	std::string m_path;

	unique_FILE make_uniqueFILE(const std::string &file, file::mode mode, error_utility::error_code error_code);

	unique_FILE m_fp_storage;
	FILE *m_fp{nullptr};
};
} // namespace io_utility
