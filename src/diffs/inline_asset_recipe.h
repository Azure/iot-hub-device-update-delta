/**
 * @file inline_asset_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class inline_asset_recipe : public recipe_base<inline_asset_recipe>
{
	public:
	inline_asset_recipe() : recipe_base_type() {}
	inline_asset_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void write(diff_writer_context &context) override;
	virtual void read(diff_reader_context &context) override;

	virtual void apply(apply_context &context) const override;
	virtual io_utility::unique_reader make_reader(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return inline_asset_recipe_name; }

	virtual uint64_t get_inline_asset_byte_count() const override;
};

class inline_asset_copy_recipe : public inline_asset_recipe
{
	virtual std::string get_recipe_type_name() const override { return inline_asset_copy_recipe_name; }
};
} // namespace diffs