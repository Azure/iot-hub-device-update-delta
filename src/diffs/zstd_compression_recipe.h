/**
 * @file zstd_compression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class zstd_compression_recipe : public diffs::recipe_base<diffs::zstd_compression_recipe>
{
	public:
	zstd_compression_recipe() : recipe_base_type() {}
	zstd_compression_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const;

	virtual std::string get_recipe_type_name() const override { return zstd_compression_recipe_name; }
};
} // namespace diffs
