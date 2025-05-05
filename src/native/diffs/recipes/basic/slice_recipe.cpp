/**
 * @file slice_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "slice_recipe.h"

#include "kitchen.h"

namespace archive_diff::diffs::recipes::basic
{
slice_recipe::slice_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{
	if (number_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"slice_recipe: Incorrect number count. Expected 1, found " + std::to_string(number_ingredients.size()));
	}

	if (item_ingredients.size() != 1)
	{
		throw errors::user_exception(
			errors::error_code::diff_recipe_invalid_parameter_count,
			"slice_recipe: Incorrect item count. Expected 1, found " + std::to_string(item_ingredients.size()));
	}

	m_offset     = number_ingredients[0];
	m_whole_item = item_ingredients[0];
}

diffs::core::recipe::prepare_result slice_recipe::prepare(
	kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const
{
	std::shared_ptr<prepared_item> whole_item_prepared = items[0];

#if 0
	if (!whole_item_prepared->can_slice(m_offset, m_result_item_definition.size()))
	{
		kitchen->request_slice(whole_item_prepared, m_offset, m_result_item_definition);

		return std::make_shared<prepared_item>(
			m_result_item_definition, prepared_item::fetch_slice_kind{kitchen->get_weak()});
	}
#else
	if (!whole_item_prepared->can_slice(m_offset, m_result_item_definition.size()))
	{
		whole_item_prepared = kitchen->prepare_as_reader(whole_item_prepared);
		if (!whole_item_prepared->can_slice(m_offset, m_result_item_definition.size()))
		{
			throw errors::user_exception(errors::error_code::diffs_kitchen_item_not_ready_to_fetch);
		}
	}
#endif

	diffs::core::prepared_item::slice_kind slice;
	slice.m_item   = whole_item_prepared;
	slice.m_offset = m_offset;
	slice.m_length = m_result_item_definition.size();

	return std::make_shared<prepared_item>(m_result_item_definition, slice);
}
} // namespace archive_diff::diffs::recipes::basic