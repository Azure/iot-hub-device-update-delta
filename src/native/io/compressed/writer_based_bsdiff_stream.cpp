/**
 * @file writer_based_bsdiff_stream.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "writer_based_bsdiff_stream.h"

namespace archive_diff::io::compressed
{
writer_based_bsdiff_stream::writer_based_bsdiff_stream(io::writer *writer) : m_writer(writer)
{
	this->seek  = seek_impl;
	this->tell  = tell_impl;
	this->read  = nullptr;
	this->write = write_impl;
	this->flush = flush_impl;
	this->close = close_impl;

	state = this;
}

int writer_based_bsdiff_stream::seek_impl(void *state, int64_t offset, int origin)
{
	auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);

	switch (origin)
	{
	case SEEK_SET:
		thisPtr->m_offset = offset;
		break;
	case SEEK_CUR:
		thisPtr->m_offset += offset;
		break;
	case SEEK_END:
		auto &writer      = thisPtr->m_writer;
		thisPtr->m_offset = writer->size() + offset;
		break;
	}

	return BSDIFF_SUCCESS;
}

int writer_based_bsdiff_stream::tell_impl(void *state, int64_t *position)
{
	auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
	*position    = thisPtr->m_offset;

	return BSDIFF_SUCCESS;
}

int writer_based_bsdiff_stream::write_impl(void *state, const void *buffer, size_t size)
{
	auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
	auto writer  = thisPtr->m_writer;
	auto offset  = thisPtr->m_offset;
	auto data    = std::string_view{reinterpret_cast<const char *>(buffer), size};

	writer->write(offset, data);
	thisPtr->m_offset += size;

	return BSDIFF_SUCCESS;
}

int writer_based_bsdiff_stream::flush_impl(void *state)
{
	auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
	auto writer  = thisPtr->m_writer;
	writer->flush();

	return BSDIFF_SUCCESS;
}
} // namespace archive_diff::io::compressed