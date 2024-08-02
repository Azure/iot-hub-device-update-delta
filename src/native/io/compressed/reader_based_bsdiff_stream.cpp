/**
 * @file reader_based_bsdiff_stream.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "reader_based_bsdiff_stream.h"

namespace archive_diff::io::compressed
{
reader_based_bsdiff_stream::reader_based_bsdiff_stream(io::reader &reader) : m_reader(reader)
{
	this->seek  = seek_impl;
	this->tell  = tell_impl;
	this->read  = read_impl;
	this->write = nullptr; // we are readonly so we shouldn't implement write/flush
	this->flush = nullptr;
	this->close = close_impl;

	state = this;
}

int reader_based_bsdiff_stream::seek_impl(void *state, int64_t offset, int origin)
{
	auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);

	switch (origin)
	{
	case SEEK_SET:
		thisPtr->m_offset = offset;
		break;
	case SEEK_CUR:
		thisPtr->m_offset += offset;
		break;
	case SEEK_END:
		auto &reader      = thisPtr->m_reader;
		thisPtr->m_offset = reader.size() + offset;
		break;
	}

	return BSDIFF_SUCCESS;
}

int reader_based_bsdiff_stream::tell_impl(void *state, int64_t *position)
{
	auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);
	*position    = thisPtr->m_offset;

	return BSDIFF_SUCCESS;
}

int reader_based_bsdiff_stream::read_impl(void *state, void *buffer, size_t size, size_t *readed)
{
	auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);
	auto reader  = thisPtr->m_reader;
	auto offset  = thisPtr->m_offset;

	*readed = reader.read_some(offset, std::span<char>(reinterpret_cast<char *>(buffer), size));

	return BSDIFF_SUCCESS;
}
} // namespace archive_diff::io::compressed