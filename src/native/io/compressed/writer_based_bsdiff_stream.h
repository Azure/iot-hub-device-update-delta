/**
 * @file writer_based_bsdiff_stream.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <bsdiff.h>

#include <io/writer.h>

namespace archive_diff::io::compressed
{
class writer_based_bsdiff_stream : public bsdiff_stream
{
	public:
	writer_based_bsdiff_stream()          = delete;
	virtual ~writer_based_bsdiff_stream() = default;

	writer_based_bsdiff_stream(writer *writer);

	static int seek_impl(void *state, int64_t offset, int origin);
	static int tell_impl(void *state, int64_t *position);
	static int write_impl(void *state, const void *buffer, size_t size);
	static int flush_impl(void *state);
	static void close_impl([[maybe_unused]] void *state) {}

	private:
	writer *m_writer{};
	int64_t m_offset{};
};
} // namespace archive_diff::io::compressed