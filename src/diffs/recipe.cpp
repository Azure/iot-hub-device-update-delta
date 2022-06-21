/**
 * @file recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <functional>

#include "recipe_type.h"
#include "archive_item_type.h"

#include "recipe.h"

#include "diff.h"
#include "recipe_parameter.h"
#include "archive_item.h"
#include "apply_context.h"

#include "hash_utility.h"
#include "binary_file_readerwriter.h"
#include "temp_file_backed_reader.h"
#include "wrapped_readerwriter.h"

#include "recipe_helpers.h"
#include "number_helpers.h"

void diffs::recipe::write(diff_writer_context &context)
{
	context.write(m_type);

	auto parameter_count = convert_number<uint8_t>(
		m_parameters.size(), error_utility::error_code::diff_recipe_parameter_count_too_large, "Too many parameters.");

	context.write(parameter_count);

	for (size_t i = 0; i < m_parameters.size(); i++)
	{
		m_parameters[i].write(context);
	}
}

// void diffs::recipe::verify(diff_resources_context &context) const
//{
//	if (m_type != recipe_type::remainder_chunk)
//	{
//		return;
//	}
//
//	// TODO: Implement? Can probably get away with hashing the overall remainder data at a higher level against a known
//	// hash instead
//}

uint64_t diffs::recipe::get_inline_asset_byte_count() const
{
	if (m_type == recipe_type::inline_asset)
	{
		uint64_t inline_asset_length = m_parameters[RECIPE_PARAMETER_LENGTH].get_number_value();
		return inline_asset_length;
	}

	uint64_t total_bytes = 0;
	for (const auto &param : m_parameters)
	{
		total_bytes += param.get_inline_asset_byte_count();
	}
	return total_bytes;
}

void diffs::recipe::add_parameter(recipe_parameter &&parameter) { m_parameters.emplace_back(std::move(parameter)); }

diffs::recipe_parameter *diffs::recipe::add_archive_item_parameter(
	diffs::archive_item_type type,
	uint64_t offset,
	uint64_t length,
	hash_type hash_type,
	const char *hash_value,
	size_t hash_value_length)
{
	m_parameters.emplace_back(
		recipe_parameter{archive_item{offset, length, type, hash_type, hash_value, hash_value_length}});

	return &m_parameters.back();
}

void diffs::recipe::add_number_parameter(uint64_t number) { m_parameters.push_back(recipe_parameter{number}); }

void diffs::recipe::prep_blob_cache(diffs::apply_context &context) const
{
	for (auto &param : m_parameters)
	{
		auto type = param.get_type();

		if (type != recipe_parameter_type::archive_item)
		{
			continue;
		}

		auto item = param.get_archive_item_value();
		item->prep_blob_cache(context);
	}
}

io_utility::unique_reader diffs::recipe::make_reader(apply_context &context) const
{
	return make_reader_using_apply(*this, context);
}

void diffs::recipe::verify_parameter_count(size_t count) const
{
	if (m_parameters.size() != count)
	{
		std::string msg = "Invalid parameter count for recipe type: " + std::to_string(static_cast<int>(m_type))
		                + " Expected: " + std::to_string(count) + ", Found: " + std::to_string(m_parameters.size());
		throw error_utility::user_exception(error_utility::error_code::diff_recipe_invalid_parameter_count, msg);
	}
}