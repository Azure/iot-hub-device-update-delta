/**
 * @file region_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class region_recipe : public recipe
{
	public:
	region_recipe(const blob_definition &blobdef) : recipe(recipe_type::region, blobdef) {}
	virtual void apply(apply_context &context) const;
};
} // namespace diffs
