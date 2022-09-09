/**
 * @file nested_diff_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class nested_diff_recipe : public recipe_base<nested_diff_recipe>
{
	public:
	nested_diff_recipe() : recipe_base_type() {}
	nested_diff_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void write(diff_writer_context &context) override;
	virtual void read(diff_reader_context &context) override;

	virtual std::string get_recipe_type_name() const override { return nested_diff_recipe_name; }

	virtual void apply(apply_context &context) const;
	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;
};
} // namespace diffs