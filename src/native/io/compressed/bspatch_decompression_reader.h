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
	bspatch_decompression_reader(io::reader &diff_reader, uint64_t uncompressed_size, io::reader &dictionary_reader);
	virtual ~bspatch_decompression_reader();

	virtual void skip(uint64_t to_skip) { m_channel->skip(to_skip); }
	virtual size_t read_some(std::span<char> buffer) override { return m_channel->read_some(buffer); }
	virtual uint64_t tellg() const override { return m_channel->tellg(); }
	virtual uint64_t size() const override { return m_channel->size(); }

	private:
	static void apply_patch_worker(
		io::reader *old_reader,
		std::shared_ptr<io::sequential::writer> *new_writer,
		io::reader *diff_reader,
		bspatch_decompression_reader *self);

	static void apply_patch(
		io::reader *old_reader, std::shared_ptr<io::sequential::writer> *new_writer, io::reader *diff_reader);

	io::reader m_diff_reader;
	io::reader m_dictionary_reader;

	std::shared_ptr<writer_to_reader_channel> m_channel;
	std::shared_ptr<io::sequential::writer> m_channel_as_writer;

	std::thread m_channel_thread;
};
} // namespace archive_diff::io::compressed