/**
 * @file apply_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>

#include "hash_utility.h"
#include "hash.h"

#include "reader.h"
#include "writer.h"
#include "readerwriter.h"
#include "hashed_writer.h"
#include "child_reader.h"

#include "blob_cache.h"

#include "diff_resources_context.h"
#include "target_context.h"
#include "source_context.h"

namespace diffs
{
class diff;

class apply_context : public diff_resources_context, public source_context, public target_context
{
	public:
	struct root_context_parameters
	{
		diffs::diff *m_diff{};
		io_utility::reader *m_diff_reader;
		std::string m_source_file{};
		std::string m_target_file{};
		blob_cache *m_blob_cache{};
		uint64_t m_length;
	};

	public:
	static apply_context root_context(const root_context_parameters &parameters) { return apply_context(parameters); }

	// a context that writes and writes to all of the same readers as the parent
	static apply_context child_context(apply_context *parent_context, uint64_t offset_in_parent, uint64_t length)
	{
		return apply_context{parent_context, offset_in_parent, length};
	}

	// a context that uses the same inputs, but doesn't write to the same location
	static apply_context same_input_context(
		apply_context *parent_context, io_utility::unique_readerwriter &&target_readerwriter, uint64_t length)
	{
		return apply_context{parent_context, std::move(target_readerwriter), length};
	}

	// a context that uses the same underlying target stream but writes to a different base offset
	static apply_context nested_context(
		apply_context *parent_context,
		io_utility::unique_reader &&source_reader,
		io_utility::unique_reader &&inline_asset_reader,
		io_utility::unique_sequential_reader &&remainder_reader,
		uint64_t length)
	{
		return apply_context{
			parent_context,
			std::move(source_reader),
			std::move(inline_asset_reader),
			std::move(remainder_reader),
			length};
	}

	static apply_context zstd_compressed_context(
		apply_context *parent_context, int major_version, int minor_version, int level, uint64_t uncompressed_size)
	{
		return apply_context(
			parent_context, compressed_context_type::zstd, major_version, minor_version, level, uncompressed_size);
	}

	apply_context(apply_context &) = default;

	private:
	// used for new root context
	apply_context(const struct root_context_parameters &parameters);

	// used for child_context()
	apply_context(apply_context *parent_context, uint64_t offset_in_parent, uint64_t length);

	// used for same_input_context()
	apply_context(
		apply_context *parent_context, io_utility::unique_readerwriter &&target_readerwriter, uint64_t length);

	// used for nested_context()
	apply_context(
		apply_context *parent_context,
		io_utility::unique_reader &&source_reader,
		io_utility::unique_reader &&inline_assets_reader,
		io_utility::unique_sequential_reader &&remainder_reader,
		uint64_t length);

	apply_context(
		apply_context *parent_context,
		compressed_context_type type,
		int major_version,
		int minor_version,
		int level,
		uint64_t uncompressed_size);

	public:
	blob_cache *get_blob_cache() { return m_blob_cache; }

	protected:
	blob_cache *m_blob_cache{};
};
} // namespace diffs
