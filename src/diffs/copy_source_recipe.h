/**
 * @file copy_source_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class copy_source_recipe : public recipe_base<copy_source_recipe>
{
	public:
	copy_source_recipe() : recipe_base_type() {}
	copy_source_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void write(diff_writer_context &context) override;
	virtual void read(diff_reader_context &context) override;

	virtual void apply(apply_context &context) const;
	virtual io_utility::unique_reader make_reader(apply_context &context) const;

	virtual void prep_blob_cache(diffs::apply_context &context) const;

	virtual std::string get_recipe_type_name() const override { return copy_source_recipe_name; };
};
} // namespace diffs