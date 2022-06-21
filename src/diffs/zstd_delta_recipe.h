/**
 * @file zstd_delta_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "delta_base_recipe.h"

namespace diffs
{
class zstd_delta_recipe : public delta_base_recipe
{
	public:
	zstd_delta_recipe(const blob_definition &blobdef) : delta_base_recipe(recipe_type::zstd_delta, blobdef) {}
	virtual void apply_delta(apply_context &context, fs::path source, fs::path delta, fs::path target) const;
};
} // namespace diffs