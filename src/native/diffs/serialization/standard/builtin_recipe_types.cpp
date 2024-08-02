/**
 * @file builtin_recipe_types.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "builtin_recipe_types.h"

#include <diffs/core/recipe_template.h>

#include <diffs/recipes/basic/all_zeros_recipe.h>
#include <diffs/recipes/basic/chain_recipe.h>
#include <diffs/recipes/basic/slice_recipe.h>

#include <diffs/recipes/compressed/bspatch_decompression_recipe.h>
#include <diffs/recipes/compressed/zlib_compression_recipe.h>
#include <diffs/recipes/compressed/zlib_decompression_recipe.h>
#include <diffs/recipes/compressed/zstd_compression_recipe.h>
#include <diffs/recipes/compressed/zstd_decompression_recipe.h>

namespace archive_diff::diffs::serialization::standard
{
template <typename RecipeT>
void ensure_builtin_recipe(core::archive *archive)
{
	std::shared_ptr<core::recipe_template> new_template = std::make_shared<typename RecipeT::recipe_template>();
	archive->set_recipe_template_for_recipe_type(RecipeT::c_recipe_name, new_template);
}

void ensure_builtin_recipe_types(core::archive *archive)
{
	ensure_builtin_recipe<diffs::recipes::basic::all_zeros_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::basic::chain_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::basic::slice_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::compressed::bspatch_decompression_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::compressed::zlib_compression_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::compressed::zlib_decompression_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::compressed::zstd_compression_recipe>(archive);
	ensure_builtin_recipe<diffs::recipes::compressed::zstd_decompression_recipe>(archive);
}
} // namespace archive_diff::diffs::serialization::standard