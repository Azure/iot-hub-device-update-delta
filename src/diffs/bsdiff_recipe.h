/**
 * @file bsdiff_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class bsdiff_recipe : public diffs::recipe_base<diffs::bsdiff_recipe>
{
	public:
	bsdiff_recipe() : recipe_base_type() {}
	bsdiff_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return bsdiff_recipe_name; };
};
} // namespace diffs