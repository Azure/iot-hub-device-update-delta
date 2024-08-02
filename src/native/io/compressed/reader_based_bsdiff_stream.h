/**
 * @file reader_based_bsdiff_stream.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <bsdiff.h>

#include <io/reader.h>

namespace archive_diff::io::compressed
{
class reader_based_bsdiff_stream : public bsdiff_stream
{
	public:
	reader_based_bsdiff_stream()          = delete;
	virtual ~reader_based_bsdiff_stream() = default;

	reader_based_bsdiff_stream(io::reader &reader);

	static int seek_impl(void *state, int64_t offset, int origin);
	static int tell_impl(void *state, int64_t *position);
	static int read_impl(void *state, void *buffer, size_t size, size_t *readed);
	static int write_impl(
		[[maybe_unused]] void *state, [[maybe_unused]] const void *buffer, [[maybe_unused]] size_t size)
	{
		return BSDIFF_SUCCESS;
	}
	static int flush_impl([[maybe_unused]] void *state) { return BSDIFF_SUCCESS; }
	static void close_impl([[maybe_unused]] void *state) {}

	private:
	io::reader m_reader;
	int64_t m_offset{};
};
} // namespace archive_diff::io::compressed