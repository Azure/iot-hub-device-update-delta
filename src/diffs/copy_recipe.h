/**
 * @file copy_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class copy_recipe : public recipe_base<copy_recipe>
{
	public:
	copy_recipe() : recipe_base_type() {}
	copy_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const;
	virtual io_utility::unique_reader make_reader(apply_context &context) const;

	virtual std::string get_recipe_type_name() const override { return copy_recipe_name; };
};
} // namespace diffs