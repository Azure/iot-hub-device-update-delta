/**
 * @file temp_file_backed_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "temp_file_backed_reader.h"

#include "binary_file_reader.h"

#include "adu_log.h"

#include "binary_file_readerwriter.h"

#ifdef WIN32

	#include <windows.h>
	#include <fileapi.h>

io_utility::temp_file_backed_reader::temp_file_backed_reader(std::function<void(io_utility::readerwriter &)> populate)
{
	m_temp_file_path = make_temp_file_path();

	static int x = 0;

	ADU_LOG("temp_file_backed_reader: %d at %s", x++, m_temp_file_path.string().c_str());

	{
		io_utility::binary_file_readerwriter readerwriter(m_temp_file_path);
		populate(readerwriter);
	}

	m_reader = std::make_unique<io_utility::binary_file_reader>(m_temp_file_path);
}

io_utility::temp_file_backed_reader::~temp_file_backed_reader()
{
	m_reader = nullptr;
	DeleteFileA(m_temp_file_path.c_str());
}

std::string io_utility::temp_file_backed_reader::make_temp_file_path()
{
	const size_t INITIAL_BUFFER_SIZE = 260; // based on MAX_PATH from windows

	std::vector<char> path_value;
	path_value.reserve(INITIAL_BUFFER_SIZE);

	while (true)
	{
		auto ret = tmpnam_s(path_value.data(), path_value.capacity());

		if (ret == 0)
		{
			break;
		}

		path_value.reserve(path_value.capacity() * 2 + 1);
	}

	return std::string(path_value.data());
}

#else
io_utility::temp_file_backed_reader::temp_file_backed_reader(std::function<void(io_utility::readerwriter &)> populate)
{
	FILE *fp = tmpfile();

	if (fp == nullptr)
	{
		std::string msg = "temp_file_backed_reader: tmpfile() failed. errno: " + std::to_string(errno);
		throw error_utility::user_exception(error_utility::error_code::io_temp_file_backed_reader_tmpfile_failed, msg);
	}

	auto readerwriter = std::make_unique<io_utility::binary_file_readerwriter>(fp);
	populate(*readerwriter.get());

	m_reader = std::move(readerwriter);
	m_fp.reset(fp);
}

io_utility::temp_file_backed_reader::~temp_file_backed_reader()
{
	m_reader.reset();
	m_fp.reset();
}
#endif
