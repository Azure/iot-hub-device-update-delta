/**
 * @file reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <limits>
#include <string>

#include <span>

#ifdef WIN32
	#include <winsock2.h>
	#undef max
#else
	#include "uint64_t_endian.h"

	#include <sys/socket.h>
	#include <arpa/inet.h>
#endif

#include "writer.h"

namespace archive_diff::io
{

void writer::write_uint8_t(uint64_t offset, uint8_t value)
{
	write(offset, std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}

void writer::write_uint16_t(uint64_t offset, uint16_t value)
{
	value = htons(value);
	write(offset, std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}

void writer::write_uint32_t(uint64_t offset, uint32_t value)
{
	value = htonl(value);
	write(offset, std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}

void writer::write_uint64_t(uint64_t offset, uint64_t value)
{
	value = htonll(value);
	write(offset, std::string_view{reinterpret_cast<char *>(&value), sizeof(value)});
}
} // namespace archive_diff::io