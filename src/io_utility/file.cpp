/**
 * @file file.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "file.h"

#include "user_exception.h"

io_utility::file::file(const std::string &file, io_utility::file::mode mode, error_utility::error_code error) :
	m_path(file)
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

static const char *get_mode_string(io_utility::file::mode mode)
{
	switch (mode)
	{
	case io_utility::file::mode::read:
		return MODE_READ;
	case io_utility::file::mode::write:
		return MODE_WRITE;
	case io_utility::file::mode::read_write:
		return MODE_READ_WRITE;
	}

	return nullptr;
}

io_utility::file::unique_FILE io_utility::file::make_uniqueFILE(
	const std::string &file, io_utility::file::mode mode, error_utility::error_code error_code)
{
	FILE *fp;
#ifdef WIN32
	auto result = fopen_s(&fp, file.c_str(), get_mode_string(mode));
	if (result)
	{
		std::string msg = "Result: " + std::to_string(result);
		throw error_utility::user_exception(error_code, msg);
	}
#else
	fp = fopen(file.c_str(), get_mode_string(mode));
#endif
	if (fp == nullptr)
	{
		std::string msg = "Failed to open " + file + ". Error: " + std::to_string(errno);
		throw error_utility::user_exception(error_code, msg);
	}

	return io_utility::file::unique_FILE{fp};
}

uint64_t io_utility::file::size()
{
	seek(0, SEEK_END);
	return tell();
}

void io_utility::file::write(uint64_t offset, std::string_view buffer)
{
	seek(offset, SEEK_SET);
	fwrite(buffer.data(), 1, buffer.size(), m_fp);
}

void io_utility::file::flush() { fflush(m_fp); }

size_t io_utility::file::read_some(uint64_t offset, gsl::span<char> buffer)
{
	seek(offset, SEEK_SET);
	return fread(buffer.data(), 1, buffer.size(), m_fp);
}

uint64_t io_utility::file::tell()
{
#ifdef WIN32
	return _ftelli64(m_fp);
#else
	return ftello64(m_fp);
#endif
}

uint64_t io_utility::file::seek(int64_t offset, int origin)
{
#ifdef WIN32
	return _fseeki64(m_fp, offset, origin);
#else
	return fseeko64(m_fp, offset, origin);
#endif
}