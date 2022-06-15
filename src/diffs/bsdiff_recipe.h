/**
 * @file bsdiff_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "delta_base_recipe.h"

namespace diffs
{
class bsdiff_recipe : public recipe
{
	public:
	bsdiff_recipe(const blob_definition &blobdef) : recipe(recipe_type::bsdiff_delta, blobdef) {}
	virtual void apply(apply_context &context) const override;
};
} // namespace diffs