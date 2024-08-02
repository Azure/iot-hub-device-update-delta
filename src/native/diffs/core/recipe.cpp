/**
 * @file recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "recipe.h"

namespace archive_diff::diffs::core
{
recipe::recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	m_result_item_definition(result_item_definition),
	m_number_ingredients(number_ingredients), m_item_ingredients(item_ingredients)
{}

recipe::~recipe() {}

bool operator<(const std::shared_ptr<recipe> &lhs, const std::shared_ptr<recipe> &rhs)
{
	auto lhs_name = lhs->get_recipe_name();
	auto rhs_name = rhs->get_recipe_name();

	auto cmp = lhs_name.compare(rhs_name);

	if (cmp != 0)
	{
		return cmp < 0;
	}

	auto &lhs_item = lhs->get_result_item_definition();
	auto &rhs_item = rhs->get_result_item_definition();

	if (lhs_item != rhs_item)
	{
		return rhs_item < rhs_item;
	}

	auto &lhs_numbers = lhs->get_number_ingredients();
	auto &rhs_numbers = rhs->get_number_ingredients();

	if (lhs_numbers.size() != rhs_numbers.size())
	{
		return lhs_numbers.size() < rhs_numbers.size();
	}

	for (size_t i = 0; i < rhs_numbers.size(); i++)
	{
		auto &lhs_value = lhs_numbers[i];
		auto &rhs_value = rhs_numbers[i];

		if (lhs_value != rhs_value)
		{
			return lhs_value < rhs_value;
		}
	}

	auto &lhs_items = lhs->get_item_ingredients();
	auto &rhs_items = rhs->get_item_ingredients();

	if (lhs_items.size() != rhs_items.size())
	{
		return lhs_items.size() < rhs_items.size();
	}

	for (size_t i = 0; i < rhs_items.size(); i++)
	{
		auto &lhs_value = lhs_items[i];
		auto &rhs_value = rhs_items[i];

		if (lhs_value != rhs_value)
		{
			return lhs_value < rhs_value;
		}
	}

	return false;
}

std::string recipe::to_string() const
{
	std::string str = "{" + get_recipe_name() + ": result: ";
	str += m_result_item_definition.to_string();
	str += ", {ingredients: {numbers: {";

	auto numbers = get_number_ingredients();
	if (numbers.size() != 0)
	{
		for (size_t i = 0; i < numbers.size() - 1; i++)
		{
			str += std::to_string(numbers[i]);
			str += ", ";
		}

		str += std::to_string(numbers.back());
	}

	str += "}, items: {";

	auto items = get_item_ingredients();
	if (items.size() != 0)
	{
		for (size_t i = 0; i < items.size() - 1; i++)
		{
			str += items[i].to_string();
			str += ", ";
		}

		str += items.back().to_string();
	}

	str += "}}}";

	return str;
}
} // namespace archive_diff::diffs::core