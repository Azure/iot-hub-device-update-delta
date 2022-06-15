/**
 * @file zstd_delta_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "zstd_delta_recipe.h"

#include "zstd_decompression_reader.h"

void diffs::zstd_delta_recipe::apply_delta(
	apply_context &context, fs::path source, fs::path delta, fs::path target) const
{
	io_utility::binary_file_reader basis_reader(source);
	io_utility::binary_file_reader delta_reader(delta);

	io_utility::zstd_decompression_reader decompression_reader(&delta_reader, m_blobdef.m_length, &basis_reader);

	context.write_target(&decompression_reader);
}