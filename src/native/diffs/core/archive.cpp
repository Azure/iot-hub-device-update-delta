/**
 * @file archive.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "archive.h"

#include "kitchen.h"

namespace archive_diff::diffs::core
{
void archive::stock_kitchen(kitchen *kitchen)
{
	kitchen->add_cookbook(m_cookbook);
	kitchen->add_pantry(m_pantry);

	for (auto &nested : m_nested_archives)
	{
		nested.second->stock_kitchen(kitchen);
	}
}

void archive::add_recipe(std::shared_ptr<recipe> &recipe) { m_cookbook->add_recipe(recipe); }

void archive::add_nested_archive(const item_definition &result, std::shared_ptr<archive> &nested_archive)
{
	m_nested_archives.insert(std::pair{result, nested_archive});
}

bool archive::has_nested_archive(const item_definition &result) const { return m_nested_archives.count(result) > 0; }

std::shared_ptr<recipe> archive::create_recipe(
	uint32_t recipe_type_id,
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients)
{
	auto &recipe_template = m_supported_recipe_templates[recipe_type_id];

	return recipe_template->create_recipe(result_item_definition, number_ingredients, item_ingredients);
}

bool archive::try_get_supported_recipe_type_id(const std::string &name, uint32_t *recipe_type_id) const
{
	auto find_itr = m_recipe_type_name_to_type_id.find(name);

	if (find_itr == m_recipe_type_name_to_type_id.cend())
	{
		return false;
	}

	if (recipe_type_id)
	{
		*recipe_type_id = find_itr->second;
	}

	return true;
}

uint32_t archive::add_supported_recipe_type(const std::string &recipe_name)
{
	auto find_itr = m_recipe_type_name_to_type_id.find(recipe_name);
	if (find_itr != m_recipe_type_name_to_type_id.cend())
	{
		return find_itr->second;
	}

	auto new_id = static_cast<uint32_t>(m_supported_recipe_templates.size());
	m_recipe_type_name_to_type_id.insert(std::pair{recipe_name, new_id});
	m_recipe_type_id_to_type_name.insert(std::pair{new_id, recipe_name});

	return new_id;
}

void archive::set_recipe_template_for_recipe_type(
	const std::string recipe_name, std::shared_ptr<recipe_template> &recipe_template)
{
	add_supported_recipe_type(recipe_name);

	auto id = m_recipe_type_name_to_type_id[recipe_name];

	m_supported_recipe_templates.insert(std::pair{id, recipe_template});
}
} // namespace archive_diff::diffs::core