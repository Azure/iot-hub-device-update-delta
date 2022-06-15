/**
 * @file recipe_helpers.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <assert.h>

#include "recipe_helpers.h"

#include "binary_file_reader.h"
#include "binary_file_readerwriter.h"

#include "diff_reader_context.h"

#include "all_zero_recipe.h"
#include "copy_recipe.h"
#include "region_recipe.h"
#include "concatenation_recipe.h"
#include "bsdiff_recipe.h"
#include "nested_diff_recipe.h"
#include "remainder_chunk_recipe.h"
#include "inline_asset_recipe.h"
#include "copy_source_recipe.h"
#include "zstd_delta_recipe.h"
#include "zstd_compression_recipe.h"
#include "zstd_decompression_recipe.h"
#include "gz_decompression_recipe.h"

const std::string RECIPE_TYPE_STRINGS[] = {
	"copy",
	"region",
	"concatenation",
	"apply_bsdiff",
	"apply_nested",
	"remainder_chunk",
	"inline_asset",
	"copy_source",
	"apply_zstd_delta",
	"inline_asset_copy",
	"zstd_compression",
	"zstd_decompression",
	"all_zero",
	"gz_decompression",
};
bool diffs::is_valid_recipe_type(recipe_type value)
{
	return (value >= recipe_type::copy) && (value <= recipe_type::last);
}

const std::string &diffs::get_recipe_type_string(recipe_type type)
{
	if (!is_valid_recipe_type(type))
	{
		std::string msg =
			"diffs::get_recipe_type_string(): Invalid recipe type: " + std::to_string(static_cast<int>(type));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_type, msg);
	}

	return RECIPE_TYPE_STRINGS[(int)type];
}

bool diffs::recipe_needs_implicit_offset(diffs::recipe_type type)
{
	switch (type)
	{
	case recipe_type::inline_asset:
	case recipe_type::remainder_chunk:
	case recipe_type::nested_diff:
		return true;
	default:
		return false;
	}
}

size_t diffs::first_implicit_parameter(recipe_type type)
{
	if (type == recipe_type::nested_diff)
	{
		return 2;
	}
	return 0;
}

size_t diffs::implicit_parameter_count(recipe_type type)
{
	size_t count = 0;
	switch (type)
	{
	case recipe_type::inline_asset:
	case recipe_type::remainder_chunk:
	case recipe_type::nested_diff:
		count++;
	default:
		break;
	}

	switch (type)
	{
	case recipe_type::copy_source:
	case recipe_type::inline_asset:
	case recipe_type::remainder_chunk:
	case recipe_type::inline_asset_copy:
		count++;
	default:
		break;
	}

	return count;
}

void diffs::increment_offset_for_type(recipe_type type, diff_reader_context &context)
{
	auto length = context.m_current_item_blobdef.m_length;

	switch (type)
	{
	case recipe_type::inline_asset:
		context.m_inline_asset_total += length;
		break;
	case recipe_type::remainder_chunk:
		context.m_remainder_chunk_total += length;
		break;
	}
}

uint64_t diffs::get_implicit_offset(diffs::recipe_type type, diffs::diff_reader_context &context)
{
	switch (type)
	{
	case recipe_type::inline_asset:
		return context.m_inline_asset_total;
		break;
	case recipe_type::remainder_chunk:
		return context.m_remainder_chunk_total;
		break;
	case recipe_type::nested_diff:
		return context.m_chunk_table_total;
		break;
	default:
		std::string msg =
			"get_and_increment_implicit_offset(): Invalid recipe type: " + std::to_string(static_cast<int>(type));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_type, msg);
	}
}

bool diffs::recipe_needs_implicit_length(diffs::recipe_type type)
{
	switch (type)
	{
	case diffs::recipe_type::copy_source:
	case diffs::recipe_type::inline_asset:
	case diffs::recipe_type::remainder_chunk:
	case diffs::recipe_type::inline_asset_copy:
		return true;
	default:
		return false;
	}
}

diffs::recipe *diffs::create_new_recipe(recipe_type type, const blob_definition &blobdef)
{
	std::unique_ptr<recipe> recipe;
	switch (type)
	{
	case diffs::recipe_type::copy:
		recipe = std::make_unique<copy_recipe>(blobdef);
		break;
	case diffs::recipe_type::region:
		recipe = std::make_unique<region_recipe>(blobdef);
		break;
	case diffs::recipe_type::concatenation:
		recipe = std::make_unique<concatenation_recipe>(blobdef);
		break;
	case diffs::recipe_type::bsdiff_delta:
		recipe = std::make_unique<bsdiff_recipe>(blobdef);
		break;
	case diffs::recipe_type::nested_diff:
		recipe = std::make_unique<nested_diff_recipe>(blobdef);
		break;
	case diffs::recipe_type::remainder_chunk:
		recipe = std::make_unique<remainder_chunk_recipe>(blobdef);
		break;
	case diffs::recipe_type::inline_asset:
		recipe = std::make_unique<inline_asset_recipe>(blobdef);
		break;
	case diffs::recipe_type::copy_source:
		recipe = std::make_unique<copy_source_recipe>(blobdef);
		break;
	case diffs::recipe_type::zstd_delta:
		recipe = std::make_unique<zstd_delta_recipe>(blobdef);
		break;
	case diffs::recipe_type::zstd_compression:
		recipe = std::make_unique<zstd_compression_recipe>(blobdef);
		break;
	case diffs::recipe_type::zstd_decompression:
		recipe = std::make_unique<zstd_decompression_recipe>(blobdef);
		break;
	case diffs::recipe_type::all_zero:
		recipe = std::make_unique<all_zero_recipe>(blobdef);
		break;
	case diffs::recipe_type::gz_decompression:
		recipe = std::make_unique<gz_decompression_recipe>(blobdef);
		break;
	default:
		std::string msg = "diffs::recipe::create_new(): Invalid recipe type: " + std::to_string(static_cast<int>(type));
		msg += ", recipe_type::last: " + std::to_string(static_cast<int>(recipe_type::last));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_type, msg);
	}

	return recipe.release();
}

diffs::recipe *diffs::read_new_recipe(diff_reader_context &context)
{
	recipe_type type;

	context.read(&type);

	std::unique_ptr<recipe> recipe;
	recipe.reset(create_new_recipe(type, context.m_current_item_blobdef));

	uint8_t parameters_count;
	context.read(&parameters_count);

	for (uint8_t i = 0; i < parameters_count; i++)
	{
		recipe_parameter parameter;
		parameter.read(context);
		recipe->add_parameter(std::move(parameter));
	}

	if (recipe_needs_implicit_offset(type))
	{
		auto expected_params = first_implicit_parameter(type);
		if (parameters_count != expected_params)
		{
			std::string msg = "diffs::recipe::read_new() : Expected to find " + std::to_string(expected_params)
			                + " parameters, found " + std::to_string(parameters_count);
			throw error_utility::user_exception(error_utility::error_code::diff_cannot_add_implicit_offset, msg);
		}
		uint64_t offset = get_implicit_offset(type, context);
		recipe_parameter offset_parameter{offset};
		static_assert(RECIPE_PARAMETER_OFFSET == 0);
		recipe->add_parameter(std::move(offset_parameter));
		parameters_count++;
	}

	increment_offset_for_type(type, context);

	if (recipe_needs_implicit_length(type))
	{
		if (parameters_count != 1)
		{
			std::string msg =
				"diffs::recipe::read_new() : Expected to find one parameters found " + std::to_string(parameters_count);
			throw error_utility::user_exception(error_utility::error_code::diff_cannot_add_implicit_length, msg);
		}

		recipe_parameter length_parameter{context.m_current_item_blobdef.m_length};
		static_assert(RECIPE_PARAMETER_LENGTH == 1);
		recipe->add_parameter(std::move(length_parameter));
		parameters_count++;
	}

	return recipe.release();
}

int get_apply_cache_id()
{
	static int id = 0;
	return id++;
}

std::string diffs::get_apply_cache_id_string() { return std::to_string(get_apply_cache_id()); }

fs::path diffs::get_apply_cache_path(const diffs::apply_context &context)
{
	return context.get_working_folder() / "apply_cache";
}

void diffs::apply_to_file(diffs::apply_context &context, const diffs::archive_item *item, fs::path path)
{
	auto parent_path = path.parent_path();

	fs::create_directories(parent_path);

	auto readerwriter = std::make_unique<io_utility::binary_file_readerwriter>(path);

	diffs::apply_context file_context =
		diffs::apply_context::same_input_context(&context, std::move(readerwriter), item->get_length());
	item->apply(file_context);
}