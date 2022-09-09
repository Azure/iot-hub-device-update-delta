/**
 * @file zstd_decompression_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class zstd_decompression_recipe : public diffs::recipe_base<diffs::zstd_decompression_recipe>
{
	public:
	zstd_decompression_recipe() : recipe_base_type() {}
	zstd_decompression_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void apply(apply_context &context) const override;
	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return zstd_decompression_recipe_name; }
};
} // namespace diffs