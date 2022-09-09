/**
 * @file concatenation_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class concatenation_recipe : public recipe_base<diffs::concatenation_recipe>
{
	public:
	concatenation_recipe() : recipe_base_type() {}
	concatenation_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const;
	virtual io_utility::unique_reader make_reader(apply_context &context) const;

	virtual std::string get_recipe_type_name() const override { return concatenation_recipe_name; };
};
} // namespace diffs
