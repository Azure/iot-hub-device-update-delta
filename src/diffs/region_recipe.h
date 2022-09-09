/**
 * @file region_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class region_recipe : public recipe_base<region_recipe>
{
	public:
	region_recipe() : recipe_base_type() {}
	region_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return region_recipe_name; }
};
} // namespace diffs
