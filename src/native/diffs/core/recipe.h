/**
 * @file recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "item_definition.h"
#include "prepared_item.h"
namespace archive_diff::diffs::core
{
class kitchen;

class recipe
{
	public:
	using prepare_result     = std::shared_ptr<prepared_item>;
	using try_prepare_result = std::optional<prepare_result>;
	using needed_map         = std::map<item_definition, bool>;

	recipe(
		const item_definition &result_item_definition,
		const std::vector<uint64_t> &number_ingredients,
		const std::vector<item_definition> &item_ingredients);
	virtual ~recipe();
	const std::vector<uint64_t> &get_number_ingredients() const { return m_number_ingredients; }
	const std::vector<item_definition> &get_item_ingredients() const { return m_item_ingredients; }
	virtual std::string get_recipe_name() const = 0;
	virtual std::string to_string() const;

	const item_definition &get_result_item_definition() const { return m_result_item_definition; }
	virtual prepare_result prepare(kitchen *kitchen, std::vector<std::shared_ptr<prepared_item>> &items) const = 0;

	protected:
	item_definition m_result_item_definition;
	std::vector<uint64_t> m_number_ingredients;
	std::vector<item_definition> m_item_ingredients;
};

bool operator<(const std::shared_ptr<recipe> &lhs, const std::shared_ptr<recipe> &rhs);
} // namespace archive_diff::diffs::core