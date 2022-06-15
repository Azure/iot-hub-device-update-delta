/**
 * @file target_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "sequential_writer.h"
#include "writer.h"
#include "readerwriter.h"
#include "hashed_writer.h"

#include "compressed_context_type.h"

#include "hash.h"

namespace diffs
{
class target_context
{
	public:
	target_context(io_utility::unique_readerwriter &&readerwriter, uint64_t length);

	static target_context nested_context(target_context *parent_context, uint64_t length)
	{
		return target_context(parent_context, length);
	}

	// used with apply_context::child_context()
	target_context(target_context *parent_context, uint64_t offset_within_target, uint64_t length);

	target_context(
		target_context *parent_context,
		compressed_context_type type,
		int major_version,
		int minor_version,
		int level,
		uint64_t uncompressed_size);

	std::unique_ptr<io_utility::reader> get_target_reader(uint64_t offset, uint64_t length);
	std::unique_ptr<io_utility::reader> get_child_target_reader();

	diffs::hash get_target_hash();

	void write_target(io_utility::reader *reader);
	void write_target(io_utility::sequential_reader *sequential_reader);

	void flush_target() { m_target_hashed_writer->flush(); }

	uint64_t get_target_bytes_written() { return m_target_bytes_written; }
	void set_target_bytes_written(uint64_t target_bytes_written) { m_target_bytes_written = target_bytes_written; }

	uint64_t get_target_length() const { return m_length; }

	private:
	// used with nested_context
	target_context(target_context *parent_context, uint64_t length);

	uint64_t m_target_reader_base_offset{};

	uint64_t m_target_bytes_written{};

	uint64_t m_length{};

	hash_utility::hasher m_hasher{hash_utility::algorithm::SHA256};

	// We may or may not have a unique_ptr to store a given reader on this context
	// If this is a child context then the pointers will be copied directly and
	// the parent context will have the unique_ptrs.
	// Various constructor types are responsible for setting up the unique_ptrs when needed.
	io_utility::unique_reader m_target_reader_storage;
	io_utility::unique_readerwriter m_target_readerwriter_storage;
	io_utility::unique_writer m_target_writer_storage;
	io_utility::unique_hashed_writer m_target_hashed_writer_storage;

	io_utility::hashed_writer *m_target_hashed_writer{};
	io_utility::reader *m_target_reader{};

	uint64_t m_offset_in_parent{};
};
} // namespace diffs
