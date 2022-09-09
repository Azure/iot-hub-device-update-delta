/**
 * @file copy_source_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "copy_source_recipe.h"

#include "recipe_helpers.h"
#include "diff_reader_context.h"
#include "diff_writer_context.h"

void diffs::copy_source_recipe::write(diff_writer_context &context)
{
	uint8_t parameter_count = 1;
	context.write(parameter_count);

	m_parameters[0].write(context);
}

void diffs::copy_source_recipe::read(diff_reader_context &context)
{
	read_parameters(context);

	recipe_parameter length_parameter{context.m_current_item_blobdef.m_length};
	add_parameter(std::move(length_parameter));
}

void diffs::copy_source_recipe::apply(apply_context &context) const
{
	auto reader = make_reader(context);
	context.write_target(reader.get());
}

std::unique_ptr<io_utility::reader> diffs::copy_source_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(2);

	uint64_t source_offset = m_parameters[RECIPE_PARAMETER_OFFSET].get_number_value();
	if (source_offset > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_copy_source_offset_too_large,
			"Copy source offset too large",
			source_offset);
	}

	uint64_t source_length = m_parameters[RECIPE_PARAMETER_LENGTH].get_number_value();
	if (source_length > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_copy_source_length_too_large,
			"Copy source length too large",
			source_length);
	}

	auto source_reader = context.get_source_reader();
	if (source_reader->get_read_style() == io_utility::reader::read_style::random_access)
	{
		std::unique_ptr<io_utility::child_reader> child_reader =
			std::make_unique<io_utility::child_reader>(source_reader, source_offset, source_length);

		return child_reader;
	}

	auto blob_cache = context.get_blob_cache();

	if (!blob_cache->is_blob_reader_available(m_blobdef) && !blob_cache->has_blob_source(source_reader, m_blobdef))
	{
		auto hash_string = m_blobdef.m_hashes[0].get_data_string();
		std::string msg  = "diffs::recipe_copy_source::make_reader(): Can't find or make reader. Hash: " + hash_string;
		throw error_utility::user_exception(error_utility::error_code::diff_no_way_to_make_source_reader, msg);
	}

	return blob_cache->wait_for_reader(m_blobdef);
}

void diffs::copy_source_recipe::prep_blob_cache(diffs::apply_context &context) const
{
	verify_parameter_count(2);

	uint64_t source_offset = m_parameters[RECIPE_PARAMETER_OFFSET].get_number_value();
	if (source_offset > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_copy_source_offset_too_large,
			"Copy source offset too large",
			source_offset);
	}

	uint64_t source_length = m_parameters[RECIPE_PARAMETER_LENGTH].get_number_value();
	if (source_length > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_copy_source_length_too_large,
			"Copy source length too large",
			source_length);
	}

	auto source_reader = context.get_source_reader();

	if (source_reader->get_read_style() == io_utility::reader::read_style::random_access)
	{
		return;
	}

	auto blob_cache = context.get_blob_cache();

	blob_cache->add_blob_request(m_blobdef);
	blob_cache->add_blob_source(source_reader, source_offset, m_blobdef);
}
