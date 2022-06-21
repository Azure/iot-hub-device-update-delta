/**
 * @file nested_diff_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class nested_diff_recipe : public recipe
{
	public:
	nested_diff_recipe(const blob_definition &blobdef) : recipe(recipe_type::nested_diff, blobdef) {}
	virtual void apply(apply_context &context) const override;
	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const override;
};
} // namespace diffs