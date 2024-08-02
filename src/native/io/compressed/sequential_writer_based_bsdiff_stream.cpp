/**
 * @file sequential_writer_based_bsdiff_stream.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "sequential_writer_based_bsdiff_stream.h"

namespace archive_diff::io::compressed
{
sequential_writer_based_bsdiff_stream::sequential_writer_based_bsdiff_stream(io::sequential::writer *writer) :
	m_writer(writer)
{
	this->seek  = nullptr;
	this->tell  = tell_impl;
	this->read  = nullptr;
	this->write = write_impl;
	this->flush = flush_impl;
	this->close = close_impl;

	state = this;
}

int sequential_writer_based_bsdiff_stream::tell_impl(void *state, int64_t *position)
{
	auto thisPtr = reinterpret_cast<sequential_writer_based_bsdiff_stream *>(state);
	*position    = thisPtr->m_writer->tellp();

	return BSDIFF_SUCCESS;
}

int sequential_writer_based_bsdiff_stream::write_impl(void *state, const void *buffer, size_t size)
{
	auto thisPtr = reinterpret_cast<sequential_writer_based_bsdiff_stream *>(state);
	auto writer  = thisPtr->m_writer;

	writer->write(std::string_view{reinterpret_cast<const char *>(buffer), size});

	return BSDIFF_SUCCESS;
}

int sequential_writer_based_bsdiff_stream::flush_impl(void *state)
{
	auto thisPtr = reinterpret_cast<sequential_writer_based_bsdiff_stream *>(state);
	auto writer  = thisPtr->m_writer;
	writer->flush();

	return BSDIFF_SUCCESS;
}
} // namespace archive_diff::io::compressed