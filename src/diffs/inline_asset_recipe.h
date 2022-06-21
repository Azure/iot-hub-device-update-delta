/**
 * @file inline_asset_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class inline_asset_recipe : public recipe
{
	public:
	inline_asset_recipe(const blob_definition &blobdef) : recipe(recipe_type::inline_asset, blobdef) {}
	virtual void apply(apply_context &context) const override;
	virtual io_utility::unique_reader make_reader(apply_context &context) const override;
};
} // namespace diffs