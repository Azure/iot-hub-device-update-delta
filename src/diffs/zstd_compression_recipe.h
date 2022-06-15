/**
 * @file zstd_compression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class zstd_compression_recipe : public recipe
{
	public:
	zstd_compression_recipe(const blob_definition &blobdef) : recipe(recipe_type::zstd_compression, blobdef) {}
	virtual void apply(apply_context &context) const;
};
} // namespace diffs
