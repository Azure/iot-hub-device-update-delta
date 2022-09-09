/**
 * @file diff.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <exception>
#include <vector>
#include <limits>

#include <thread>

#include "diff.h"
#include "verified_reader.h"

#include "hash_utility.h"
#include "chained_reader.h"
#include "zlib_decompression_reader.h"

#include "number_helpers.h"

#include "user_exception.h"

void diffs::diff::read(diff_reader_context &context)
{
	context.set_recipe_host(this);

	char magic[4]{};
	context.read(gsl::span<char>{magic, 4});

	if (0 != memcmp(magic, DIFF_MAGIC_VALUE, sizeof(magic)))
	{
		std::string magic_string(magic, 4);
		std::string msg = "Not a valid diff file - invalid magic. Found: " + magic_string;
		throw error_utility::user_exception(error_utility::error_code::diff_magic_header_wrong, msg);
	}

	context.read(&m_version);

	if (m_version != DIFF_VERSION)
	{
		std::string msg =
			"Not a valid version. Expected: " + std::to_string(DIFF_VERSION) + ", Found: " + std::to_string(m_version);
		throw error_utility::user_exception(error_utility::error_code::diff_version_wrong, msg);
	}

	context.read(&m_target_size);
	m_target_hash.read(context);

	context.read(&m_source_size);
	if (m_source_size != 0)
	{
		m_source_hash.read(context);
	}

	uint64_t chunk_count;
	context.read(&chunk_count);
	verify_can_convert_number<size_t, uint64_t>(
		chunk_count, error_utility::error_code::diff_chunk_count_too_large, "Diff chunk count too large.");

	context.m_chunk_table_total_stack.push(context.m_chunk_table_total);
	context.m_chunk_table_total = 0;

	uint64_t offset = 0;
	for (uint64_t i = 0; i < chunk_count; i++)
	{
		m_chunks.emplace_back(offset);
		auto &new_chunk = m_chunks.back();
		new_chunk.read(context, true);

		m_inline_assets_size += new_chunk.get_inline_asset_byte_count();

		offset += m_chunks.back().get_length();
		context.m_chunk_table_total = offset;
	}

	context.m_chunk_table_total = context.m_chunk_table_total_stack.top();
	context.m_chunk_table_total_stack.pop();

	// Read in byte count for inline_assets and compare against calculated value
	uint64_t inline_asset_byte_count_from_reader;
	context.read(&inline_asset_byte_count_from_reader);

	// N.B. if we ever have an inline asset being used more than once
	// then this will break down
	if (inline_asset_byte_count_from_reader != m_inline_assets_size)
	{
		std::string msg = "from stream: " + std::to_string(inline_asset_byte_count_from_reader)
		                + ", from chunks: " + std::to_string(m_inline_assets_size);
		throw error_utility::user_exception(error_utility::error_code::diff_inline_asset_byte_count_mismatch, msg);
	}

	m_inline_assets_offset = context.tellg();

	context.skip(m_inline_assets_size);

	context.read(&m_remainder_uncompressed_size);
	context.read(&m_remainder_size);

	m_remainder_offset = context.tellg();

	m_diff_size = m_remainder_offset + m_remainder_size;

	if (m_diff_size != context.size())
	{
		std::string msg = "diffs::read(). Size mismatch for diff. Size based on reading data: "
		                + std::to_string(m_diff_size) + ". Size from context: " + std::to_string(context.size());
		throw error_utility::user_exception(error_utility::error_code::diff_read_diff_size_mismatch, msg);
	}
}

void diffs::diff::set_remainder_sizes(uint64_t uncompressed, uint64_t compressed)
{
	m_remainder_uncompressed_size = uncompressed;
	m_remainder_size              = compressed;
}

void diffs::diff::write(diffs::diff_writer_context &context)
{
	context.set_recipe_host(this);

	context.write_raw(std::string_view{DIFF_MAGIC_VALUE, 4});
	context.write_raw(DIFF_VERSION);

	context.write(m_target_size);
	m_target_hash.write(context);

	context.write(m_source_size);
	if (m_source_size != 0)
	{
		m_source_hash.write(context);
	}

	context.write(static_cast<uint64_t>(m_chunks.size()));

	uint64_t expected_chunk_offset            = 0;
	uint64_t expected_inline_asset_byte_count = 0;
	m_inline_assets_size                      = context.get_inline_asset_byte_count();

	for (uint64_t i = 0; i < m_chunks.size(); i++)
	{
		auto &chunk = m_chunks[i];
		chunk.write(context, true);

		if (chunk.get_offset() != expected_chunk_offset)
		{
			std::string msg = "Attempting to write chunk at incorrect offset. Expected: "
			                + std::to_string(expected_chunk_offset) + " Actual: " + std::to_string(chunk.get_offset());
			throw error_utility::user_exception(error_utility::error_code::diff_write_chunk_at_unexpected_offset, msg);
		}

		expected_chunk_offset += chunk.get_length();
		expected_inline_asset_byte_count += chunk.get_inline_asset_byte_count();
	}

	if (expected_inline_asset_byte_count != m_inline_assets_size)
	{
		std::string msg =
			"Found incorrect amount of inline assets. Expected: " + std::to_string(expected_inline_asset_byte_count)
			+ " Actual: " + std::to_string(m_inline_assets_size);
		throw error_utility::user_exception(error_utility::error_code::diff_write_incorrect_inline_assets, msg);
	}

	context.write_raw(m_inline_assets_size);

	context.write_inline_assets();

	context.write_raw(m_remainder_uncompressed_size);
	context.write_raw(m_remainder_size);

	context.write_remainder();
}

std::unique_ptr<io_utility::reader> diffs::diff::make_inline_assets_reader(io_utility::reader *reader) const
{
	return std::make_unique<io_utility::child_reader>(reader, m_inline_assets_offset, m_inline_assets_size);
}

std::unique_ptr<io_utility::sequential_reader> diffs::diff::make_remainder_reader(io_utility::reader *reader) const
{
	auto raw_remainder_reader =
		std::make_unique<io_utility::child_reader>(reader, m_remainder_offset, m_remainder_size);
	return std::make_unique<io_utility::zlib_decompression_reader>(
		std::move(raw_remainder_reader),
		m_remainder_uncompressed_size,
		io_utility::zlib_decompression_reader::init_type::raw);
}

void populate_from_reader(diffs::blob_cache *cache, io_utility::reader *reader)
{
	if (reader->get_read_style() == io_utility::reader::read_style::sequential_only)
	{
		cache->populate_from_reader(reader);
	}

	cache->clear_blob_locations_for_reader(reader);
}

diffs::diff::verify_source_result diffs::diff::verify_source(io_utility::reader &reader) const
{
	if (reader.size() != m_source_size)
	{
		std::string msg = "diffs::diff::verify_source(): Source size mismatch. Actual " + std::to_string(reader.size())
		                + " Expected: " + std::to_string(m_source_size);
		throw error_utility::user_exception(error_utility::error_code::diff_verify_source_size_mismatch, msg);
	}

	if (reader.get_read_style() != io_utility::reader::read_style::random_access)
	{
		return verify_source_result::uncertain;
	}

	hash actual_hash{m_source_hash.m_hash_type, reader};

	hash::verify_hashes_match(actual_hash, m_source_hash);

	return verify_source_result::identical;
}

void diffs::diff::apply(diffs::apply_context &context)
{
	for (size_t i = 0; i < m_chunks.size(); i++)
	{
		auto &chunk = m_chunks[i];
		chunk.prep_blob_cache(context);
	}

	std::thread thread(populate_from_reader, context.get_blob_cache(), context.get_source_reader());

	try
	{
		for (size_t i = 0; i < m_chunks.size(); i++)
		{
			auto &chunk = m_chunks[i];
			chunk.apply(context);
		}

		auto actual_hash = context.get_target_hash();
		diffs::hash::verify_hashes_match(actual_hash, m_target_hash);

		context.flush_target();
	}
	catch (std::exception &)
	{
		context.get_blob_cache()->cancel();
		thread.join();
		throw;
	}
	catch (error_utility::user_exception &)
	{
		context.get_blob_cache()->cancel();
		thread.join();
		throw;
	}

	thread.join();
}

io_utility::unique_reader diffs::diff::make_reader(apply_context &context) const
{
	for (size_t i = 0; i < m_chunks.size(); i++)
	{
		auto &chunk = m_chunks[i];
		chunk.prep_blob_cache(context);
	}

	std::thread thread(populate_from_reader, context.get_blob_cache(), context.get_source_reader());

	std::vector<io_utility::unique_reader> readers;

	for (size_t i = 0; i < m_chunks.size(); i++)
	{
		auto &chunk = m_chunks[i];
		readers.emplace_back(std::make_unique<verified_reader>(
			chunk.get_offset(), std::move(chunk.make_reader(context)), chunk.get_hash()));

		if (chunk.get_length() != readers.back()->size())
		{
			context.get_blob_cache()->cancel();
			thread.join();
			std::string msg = "Chunk and reader at offset " + std::to_string(i)
			                + " mismatch lengths. "
			                  "Chunk length: "
			                + std::to_string(chunk.get_length())
			                + "Reader size: " + std::to_string(readers.back()->size());
			throw error_utility::user_exception(error_utility::error_code::diff_version_wrong, msg);
		}
	}

	thread.join();

	return std::make_unique<io_utility::chained_reader>(std::move(readers));
}
