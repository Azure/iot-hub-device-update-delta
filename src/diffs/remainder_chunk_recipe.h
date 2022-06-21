/**
 * @file remainder_chunk_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class remainder_chunk_recipe : public recipe
{
	public:
	remainder_chunk_recipe(const blob_definition &blobdef) : recipe(recipe_type::remainder_chunk, blobdef) {}
	virtual void apply(apply_context &context) const;
};
} // namespace diffs