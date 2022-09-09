/**
 * @file gz_decompression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class gz_decompression_recipe : public recipe_base<gz_decompression_recipe>
{
	public:
	gz_decompression_recipe() : recipe_base_type() {}
	gz_decompression_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const override;
	virtual io_utility::unique_reader make_reader(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return gz_decompression_recipe_name; };
};
} // namespace diffs