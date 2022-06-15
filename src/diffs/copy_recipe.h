/**
 * @file copy_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class copy_recipe : public recipe
{
	public:
	copy_recipe(const blob_definition &blobdef) : recipe(recipe_type::copy, blobdef) {}
	virtual void apply(apply_context &context) const;
	virtual io_utility::unique_reader make_reader(apply_context &context) const override;
};
} // namespace diffs