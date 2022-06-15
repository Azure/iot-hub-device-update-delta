/**
 * @file zstd_decompression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class zstd_decompression_recipe : public recipe
{
	public:
	zstd_decompression_recipe(const blob_definition &blobdef) : recipe(recipe_type::zstd_decompression, blobdef) {}
	virtual void apply(apply_context &context) const;

	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;
};
} // namespace diffs