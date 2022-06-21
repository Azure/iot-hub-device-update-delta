/**
 * @file delta_base_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class delta_base_recipe : public recipe
{
	public:
	delta_base_recipe(recipe_type type, const blob_definition &blobdef) : recipe(type, blobdef) {}
	virtual void apply(apply_context &context) const;
	virtual void apply_delta(apply_context &context, fs::path source, fs::path delta, fs::path target) const = 0;
};
} // namespace diffs