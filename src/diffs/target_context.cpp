/**
 * @file target_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "target_context.h"

#include "sequential_reader.h"
#include "child_reader.h"
#include "child_hashed_writer.h"
#include "wrapped_writer_sequential_hashed_writer.h"

#include "zstd_compression_writer.h"

// used for apply_context::same_input_context()
diffs::target_context::target_context(io_utility::unique_readerwriter &&readerwriter, uint64_t length) :
	m_target_readerwriter_storage(std::move(readerwriter)),
	m_target_hashed_writer_storage(std::make_unique<io_utility::wrapped_writer_sequential_hashed_writer>(
		m_target_readerwriter_storage.get(), &m_hasher)),
	m_target_hashed_writer(m_target_hashed_writer_storage.get()), m_target_reader(m_target_readerwriter_storage.get()),
	m_length(length)
{}

// used with nested_context
diffs::target_context::target_context(target_context *parent_context, uint64_t length) :
	m_target_hashed_writer_storage(
		std::make_unique<io_utility::child_hashed_writer>(parent_context->m_target_hashed_writer, &m_hasher)),
	m_target_hashed_writer(m_target_hashed_writer_storage.get()),
	m_target_reader_storage(parent_context->get_child_target_reader()), m_target_reader(m_target_reader_storage.get()),
	m_length(length)
{}

// used with apply_context::child_context()
diffs::target_context::target_context(target_context *parent_context, uint64_t offset_within_target, uint64_t length) :
	m_target_reader(parent_context->m_target_reader),
	m_target_hashed_writer_storage(
		std::make_unique<io_utility::child_hashed_writer>(parent_context->m_target_hashed_writer, &m_hasher)),
	m_target_hashed_writer(m_target_hashed_writer_storage.get()), m_offset_in_parent(offset_within_target),
	m_length(length)
{}

diffs::target_context::target_context(
	target_context *parent_context,
	compressed_context_type type,
	int major_version,
	int minor_version,
	int level,
	uint64_t uncompressed_size)
{
	m_target_writer_storage = std::make_unique<io_utility::zstd_compression_writer>(
		parent_context->m_target_hashed_writer, major_version, minor_version, level, uncompressed_size);
	m_target_hashed_writer_storage =
		std::make_unique<io_utility::wrapped_writer_sequential_hashed_writer>(m_target_writer_storage.get(), &m_hasher);

	if (type != compressed_context_type::zstd)
	{
		std::string msg =
			"apply_context::apply_context(): compressed_context_type doesn't match compressed_context_type::zstd. "
			"Actual value: "
			+ std::to_string(static_cast<int>(type));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_compression_context_type, msg);
	}
	m_target_hashed_writer = m_target_hashed_writer_storage.get();
}

void diffs::target_context::write_target(io_utility::sequential_reader *sequential_reader)
{
	m_target_hashed_writer->write(sequential_reader);
	m_target_bytes_written += sequential_reader->size();
}

void diffs::target_context::write_target(io_utility::reader *reader)
{
	m_target_hashed_writer->write(reader);
	m_target_bytes_written += reader->size();
}

diffs::hash diffs::target_context::get_target_hash()
{
	return diffs::hash{diffs::hash_type::Sha256, m_hasher.get_hash_binary()};
}

std::unique_ptr<io_utility::reader> diffs::target_context::get_target_reader(uint64_t offset, uint64_t length)
{
	return std::make_unique<io_utility::child_reader>(m_target_reader, offset, length);
}

std::unique_ptr<io_utility::reader> diffs::target_context::get_child_target_reader()
{
	return io_utility::child_reader::with_base_offset(m_target_reader, m_offset_in_parent);
}
