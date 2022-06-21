/**
 * @file copy_source_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"

namespace diffs
{
class copy_source_recipe : public recipe
{
	public:
	copy_source_recipe(const blob_definition &blobdef) : recipe(recipe_type::copy_source, blobdef) {}
	virtual void apply(apply_context &context) const;

	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;

	virtual void prep_blob_cache(diffs::apply_context &context) const;
};
} // namespace diffs