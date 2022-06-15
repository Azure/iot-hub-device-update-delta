/**
 * @file concatenation_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class concatenation_recipe : public recipe
{
	public:
	concatenation_recipe(const blob_definition &blobdef) : recipe(recipe_type::concatenation, blobdef) {}
	virtual void apply(apply_context &context) const;
	virtual io_utility::unique_reader make_reader(apply_context &context) const;
};
} // namespace diffs
