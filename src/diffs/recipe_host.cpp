#include "recipe_host.h"

#include "all_zero_recipe.h"
#include "bsdiff_recipe.h"
#include "concatenation_recipe.h"
#include "copy_recipe.h"
#include "copy_source_recipe.h"
#include "gz_decompression_recipe.h"
#include "inline_asset_recipe.h"
#include "nested_diff_recipe.h"
#include "region_recipe.h"
#include "remainder_chunk_recipe.h"
#include "zstd_compression_recipe.h"
#include "zstd_decompression_recipe.h"
#include "zstd_delta_recipe.h"

void diffs::recipe_host::initialize_supported_recipes()
{
	m_supported_recipes.emplace_back(std::make_unique<diffs::copy_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::region_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::concatenation_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::bsdiff_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::nested_diff_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::remainder_chunk_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::inline_asset_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::copy_source_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::zstd_delta_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::inline_asset_copy_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::zstd_compression_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::zstd_decompression_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::all_zero_recipe>());
	m_supported_recipes.emplace_back(std::make_unique<diffs::gz_decompression_recipe>());

	for (int32_t i = 0; i < m_supported_recipes.size(); i++)
	{
		auto recipe                                                       = m_supported_recipes[i].get();
		m_supported_recipe_factory_lookup[recipe->get_recipe_type_name()] = recipe;
		m_supported_recipe_id_lookup[recipe->get_recipe_type_name()]      = i;
	}
}

std::unique_ptr<diffs::recipe> diffs::recipe_host::create_recipe(
	uint32_t recipe_type_id, const blob_definition &blobdef) const
{
	if (m_supported_recipes.size() <= recipe_type_id)
	{
		throw std::exception();
	}

	return m_supported_recipes[recipe_type_id]->create(blobdef);
}

std::unique_ptr<diffs::recipe> diffs::recipe_host::create_recipe(
	const char *recipe_type_name, const blob_definition &blobdef) const
{
	if (!is_recipe_type_supported(recipe_type_name))
	{
		throw std::exception();
	}

	const auto recipe = m_supported_recipe_factory_lookup.at(recipe_type_name);

	return recipe->create(blobdef);
}

bool diffs::recipe_host::is_recipe_type_supported(const std::string &recipe_type_name) const
{
	return m_supported_recipe_factory_lookup.count(recipe_type_name) > 0;
}

int32_t diffs::recipe_host::get_recipe_type_id(const std::string &recipe_type_name) const
{
	if (m_supported_recipe_id_lookup.count(recipe_type_name) == 0)
	{
		throw std::exception();
	}

	return m_supported_recipe_id_lookup.at(recipe_type_name);
}