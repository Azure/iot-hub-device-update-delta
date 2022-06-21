/**
 * @file delta_base_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "delta_base_recipe.h"

#include "recipe_helpers.h"

void diffs::delta_base_recipe::apply(apply_context &context) const
{
	verify_parameter_count(2);

	const auto &source_param = m_parameters[RECIPE_PARAMETER_SOURCE];
	const auto &delta_param  = m_parameters[RECIPE_PARAMETER_DELTA];

	std::string id_string     = get_apply_cache_id_string();
	fs::path apply_cache_path = get_apply_cache_path(context);
	if (!fs::exists(apply_cache_path))
	{
		fs::create_directories(apply_cache_path);
	}

	fs::path basis_path  = apply_cache_path / (id_string + ".basis");
	fs::path delta_path  = apply_cache_path / (id_string + ".delta");
	fs::path target_path = apply_cache_path / (id_string + ".target");

	apply_to_file(context, delta_param.get_archive_item_value(), delta_path);
	apply_to_file(context, source_param.get_archive_item_value(), basis_path);

	apply_delta(context, basis_path, delta_path, target_path);
}