/**
 * @file bspatch_decompression_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <thread>
#include <string>

#include <bsdiff.h>

#include <io/sequential/reader.h>

#include "writer_to_reader_channel.h"

namespace archive_diff::io::compressed
{
class bspatch_decompression_reader : public io::sequential::reader
{
	public:
	bspatch_decompression_reader(io::reader &diff_reader, uint64_t uncompressed_size, io::reader &dictionary_reader) :
		m_diff_reader(diff_reader), m_dictionary_reader(dictionary_reader), m_channel(uncompressed_size)
	{
		m_channel_thread = std::thread(call_bspatch, &m_dictionary_reader, &m_diff_reader, &m_channel, this);
	}

	~bspatch_decompression_reader()
	{
		m_channel.cancel();
		m_channel_thread.join();
	}

	virtual void skip(uint64_t to_skip) { m_channel.skip(to_skip); }
	virtual size_t read_some(std::span<char> buffer) override { return m_channel.read_some(buffer); }
	virtual uint64_t tellg() const override { return m_channel.tellg(); }
	virtual uint64_t size() const override { return m_channel.size(); }

	private:
	static void call_bspatch(io::reader *basis, io::reader *delta, io::sequential::writer *sink, void *object);

	io::reader m_diff_reader;
	io::reader m_dictionary_reader;

	writer_to_reader_channel m_channel;

	std::thread m_channel_thread;
};
} // namespace archive_diff::io::compressed