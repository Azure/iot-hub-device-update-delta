/**
 * @file diff_reader_context.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diff_reader_context.h"

std::string diffs::diff_reader_context::get_recipe_type_name(uint32_t index) const
{
	if (m_recipe_type_name_map.count(index) == 0)
	{
		throw std::exception();
	}

	return m_recipe_type_name_map.at(index);
}

void diffs::diff_reader_context::set_recipe_type_name(uint32_t index, const std::string name)
{
	m_recipe_type_name_map[index] = name;
}