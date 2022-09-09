/**
 * @file remainder_chunk_recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "recipe.h"
#include "recipe_names.h"

namespace diffs
{
class remainder_chunk_recipe : public recipe_base<remainder_chunk_recipe>
{
	public:
	remainder_chunk_recipe() : recipe_base_type() {}
	remainder_chunk_recipe(const blob_definition &blobdef) : recipe_base_type(blobdef) {}

	virtual void write(diff_writer_context &context) override;
	virtual void read(diff_reader_context &context) override;

	virtual void apply(apply_context &context) const override;

	virtual std::string get_recipe_type_name() const override { return remainder_chunk_recipe_name; }
};
} // namespace diffs