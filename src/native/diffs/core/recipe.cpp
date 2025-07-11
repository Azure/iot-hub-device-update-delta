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
	m_result_item_definition(result_item_definition), m_number_ingredients(number_ingredients),
	m_item_ingredients(item_ingredients)
{
	if (std::any_of(
			m_item_ingredients.begin(),
			m_item_ingredients.end(),
			[&](const auto &item_ingredient) { return m_result_item_definition == item_ingredient; }))
	{
		throw errors::user_exception(
			errors::error_code::recipe_self_referential,
			fmt::format("Recipe is self referential for item: {}", m_result_item_definition.to_string()));
	}
}

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

Json::Value recipe::to_json() const
{
	Json::Value value;

	value["Name"]   = get_recipe_name();
	value["Result"] = m_result_item_definition.to_json();

	if (m_number_ingredients.size())
	{
		Json::Value number_ingredients;

		for (auto number : m_number_ingredients)
		{
			number_ingredients.append(number);
		}
		value["NumberIngredients"] = number_ingredients;
	}

	if (m_item_ingredients.size())
	{
		Json::Value item_ingredients;

		for (auto &item : m_item_ingredients)
		{
			item_ingredients.append(item.to_json());
		}
		value["ItemIngredients"] = item_ingredients;
	}

	return value;
}

std::string recipe::to_string() const
{
	auto json = to_json();

	std::stringstream stream;
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(json, &stream);

	return stream.str();
}
} // namespace archive_diff::diffs::core