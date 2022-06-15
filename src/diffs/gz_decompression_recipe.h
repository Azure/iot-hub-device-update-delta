/**
 * @file gz_decompression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class gz_decompression_recipe : public recipe
{
	public:
	gz_decompression_recipe(const blob_definition &blobdef) : recipe(recipe_type::gz_decompression, blobdef) {}
	virtual void apply(apply_context &context) const;

	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;
};
} // namespace diffs