/**
 * @file recipe_host.h
 * @description A recipe_host is capable of creating recipes with by name
 * or by index. The host has an internal table
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <memory>
#include <vector>
#include <map>

#include "recipe.h"

namespace diffs
{
class recipe_host
{
	public:
	recipe_host() { initialize_supported_recipes(); }

	bool is_recipe_type_supported(const std::string &name) const;
	int32_t get_recipe_type_id(const std::string &name) const;
	std::unique_ptr<recipe> create_recipe(uint32_t recipe_type_id, const blob_definition &blobdef) const;
	std::unique_ptr<recipe> create_recipe(const char *recipe_type_name, const blob_definition &blobdef) const;

	private:
	void initialize_supported_recipes();

	std::vector<std::unique_ptr<recipe>> m_supported_recipes;
	std::map<std::string, recipe *> m_supported_recipe_factory_lookup;
	std::map<std::string, uint32_t> m_supported_recipe_id_lookup;
};
} // namespace diffs