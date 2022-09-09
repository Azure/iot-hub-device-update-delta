/**
 * @file apply_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <memory>
#include <utility>

#include "diff.h"

#include "apply_context.h"
#include "hash_utility.h"

#include "zlib_decompression_reader.h"
#include "zstd_compression_writer.h"

#include "binary_file_reader.h"
#include "binary_file_readerwriter.h"

#include "child_hashed_writer.h"
#include "wrapped_writer_sequential_hashed_writer.h"

#include "user_exception.h"

// used for new root context
diffs::apply_context::apply_context(const root_context_parameters &params) :
	diff_resources_context(diff_resources_context::from_diff(params.m_diff, params.m_diff_reader)),
	source_context(std::make_unique<io_utility::binary_file_reader>(params.m_source_file)),
	target_context(std::make_unique<io_utility::binary_file_readerwriter>(params.m_target_file), params.m_length),
	m_blob_cache(params.m_blob_cache)
{}

// used for child_context()
diffs::apply_context::apply_context(apply_context *parent_context, uint64_t offset_in_parent, uint64_t length) :
	diff_resources_context(parent_context), source_context(parent_context),
	target_context(parent_context, offset_in_parent, length), m_blob_cache(parent_context->m_blob_cache)
{}

// used for same_input_context()
diffs::apply_context::apply_context(
	apply_context *parent_context, io_utility::unique_readerwriter &&target_readerwriter, uint64_t length) :
	diff_resources_context(parent_context),
	source_context(parent_context), target_context(std::move(target_readerwriter), length),
	m_blob_cache(parent_context->m_blob_cache)
{}

// used for nested_context()
diffs::apply_context::apply_context(
	apply_context *parent_context,
	io_utility::unique_reader &&source_reader,
	io_utility::unique_reader &&inline_assets_reader,
	io_utility::unique_sequential_reader &&remainder_reader,
	uint64_t length) :
	diff_resources_context(std::move(inline_assets_reader), std::move(remainder_reader)),
	source_context(std::move(source_reader)), target_context(target_context::nested_context(parent_context, length)),
	m_blob_cache(parent_context->m_blob_cache)
{}

diffs::apply_context::apply_context(
	apply_context *parent_context,
	compressed_context_type type,
	int major_version,
	int minor_version,
	int level,
	uint64_t uncompressed_size) :
	diff_resources_context(parent_context),
	source_context(parent_context),
	target_context(parent_context, type, major_version, minor_version, level, uncompressed_size),
	m_blob_cache(parent_context->m_blob_cache)
{}
