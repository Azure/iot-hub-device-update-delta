/**
 * @file diff.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <gsl/span>

#include "hash.h"

#include "archive.h"

#include "apply_context.h"
#include "diff_reader_context.h"
#include "diff_writer_context.h"

namespace diffs
{
class archive_item;
class diff : public archive
{
	public:
	diff() : archive(){};

	diff(io_utility::reader *reader) : archive()
	{
		diff_reader_context context{reader};
		read(context);
	}

	void read(diff_reader_context &context);
	void write(diff_writer_context &context);

	enum class verify_source_result
	{
		uncertain,
		identical,
	};

	verify_source_result verify_source(io_utility::reader &reader) const;

	// void dump(diff_viewer_context &context, std::ostream &ostream);

	void apply(apply_context &context);
	io_utility::unique_reader make_reader(apply_context &context) const;

	void set_remainder_sizes(uint64_t uncompressed, uint64_t compressed);
	void set_target_size(uint64_t size) { m_target_size = size; }
	void set_target_hash(hash_type hash_type, const char *hash_value, size_t hash_value_length)
	{
		m_target_hash.m_hash_type = hash_type;
		m_target_hash.m_hash_data.resize(hash_value_length);
		memcpy(m_target_hash.m_hash_data.data(), hash_value, hash_value_length);
	}
	void set_source_size(uint64_t size) { m_source_size = size; }
	void set_source_hash(hash_type hash_type, const char *hash_value, size_t hash_value_length)
	{
		m_source_hash.m_hash_type = hash_type;
		m_source_hash.m_hash_data.resize(hash_value_length);
		memcpy(m_source_hash.m_hash_data.data(), hash_value, hash_value_length);
	}

	std::unique_ptr<io_utility::reader> make_inline_assets_reader(io_utility::reader *reader) const;
	std::unique_ptr<io_utility::sequential_reader> make_remainder_reader(io_utility::reader *reader) const;

	uint64_t get_version() const { return m_version; }
	uint64_t get_target_size() const { return m_target_size; }
	hash get_target_hash() const { return m_target_hash; }
	uint64_t get_source_size() const { return m_source_size; }
	hash get_source_hash() const { return m_source_hash; }

	uint64_t get_inline_assets_offset() const { return m_inline_assets_offset; }
	uint64_t get_inline_assets_size() const { return m_inline_assets_size; }

	uint64_t get_remainder_offset() const { return m_remainder_offset; }
	uint64_t get_remainder_size() const { return m_remainder_size; }
	uint64_t get_remainder_uncompressed_size() const { return m_remainder_uncompressed_size; }

	uint64_t get_diff_size() const { return m_diff_size; }

	private:
	const char *DIFF_MAGIC_VALUE = "PAMZ";

	const uint64_t DIFF_VERSION = 0;

	uint64_t m_version{DIFF_VERSION};
	uint64_t m_target_size{};
	hash m_target_hash;

	uint64_t m_source_size{};
	hash m_source_hash;

	uint64_t m_inline_assets_offset{};
	uint64_t m_inline_assets_size{};

	uint64_t m_remainder_offset{};
	uint64_t m_remainder_size{};
	uint64_t m_remainder_uncompressed_size{};

	uint64_t m_diff_size{};
};
} // namespace diffs