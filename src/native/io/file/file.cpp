/**
 * @file file.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "file.h"

#include "user_exception.h"

namespace archive_diff::io::file
{
file::file(const std::string &file, mode mode, errors::error_code error) : m_path(file)
{
	m_fp_storage = make_uniqueFILE(file, mode, error);
	m_fp         = m_fp_storage.get();
}

#ifdef WIN32
	#define MODE_READ       "rb"
	#define MODE_WRITE      "wb"
	#define MODE_READ_WRITE "w+b"
#else
	#define MODE_READ       "rb"
	#define MODE_WRITE      "wb"
	#define MODE_READ_WRITE "w+b"
#endif

static const char *get_mode_string(io::file::file::mode mode)
{
	switch (mode)
	{
	case file::mode::read:
		return MODE_READ;
	case file::mode::write:
		return MODE_WRITE;
	case file::mode::read_write:
		return MODE_READ_WRITE;
	}

	return nullptr;
}

file::unique_FILE file::make_uniqueFILE(const std::string &file, mode mode, errors::error_code error_code)
{
	FILE *fp;
#ifdef WIN32
	auto result = fopen_s(&fp, file.c_str(), get_mode_string(mode));
	if (result)
	{
		std::string msg = "Failed to open file: ";
		msg += file;
		msg += ". Result: " + std::to_string(result);
		throw errors::user_exception(error_code, msg);
	}
#else
	fp = fopen(file.c_str(), get_mode_string(mode));
#endif
	if (fp == nullptr)
	{
		std::string msg = "Failed to open file: ";
		msg += file;
		msg += ". errno: " + std::to_string(errno);
		throw errors::user_exception(error_code, msg);
	}

	// printf("Opening: %s, FP: 0x%p\n", file.c_str(), fp);

	return file::unique_FILE{fp};
}

uint64_t file::size()
{
	std::lock_guard<std::mutex> guard(m_mutex);
	seek(0, SEEK_END);
	return tell();
}

void file::write(uint64_t offset, std::string_view buffer)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	seek(offset, SEEK_SET);
	fwrite(buffer.data(), 1, buffer.size(), m_fp);
}

void file::flush() { fflush(m_fp); }

size_t file::read_some(uint64_t offset, std::span<char> buffer)
{
	std::lock_guard<std::mutex> guard(m_mutex);
	seek(offset, SEEK_SET);
	return fread(buffer.data(), 1, buffer.size(), m_fp);
}

uint64_t file::tell()
{
#ifdef WIN32
	return _ftelli64(m_fp);
#else
	return ftello64(m_fp);
#endif
}

uint64_t file::seek(int64_t offset, int origin)
{
#ifdef WIN32
	return _fseeki64(m_fp, offset, origin);
#else
	return fseeko64(m_fp, offset, origin);
#endif
}
} // namespace archive_diff::io::file