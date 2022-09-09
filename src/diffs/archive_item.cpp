/**
 * @file archive_item.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <assert.h>

#include "user_exception.h"

#include "archive_item.h"
#include "diff.h"
#include "recipe.h"
#include "apply_context.h"
#include "diff_writer_context.h"
#include "recipe_helpers.h"

#include "hash_utility.h"

diffs::archive_item::archive_item(uint64_t offset) : m_offset(offset), m_type(archive_item_type::chunk) {}

void diffs::archive_item::read(diff_reader_context &context, bool in_chunk_table)
{
	if (in_chunk_table)
	{
		m_type = archive_item_type::chunk;
	}
	else
	{
		context.read(&m_type);
		if (m_type == archive_item_type::chunk)
		{
			context.read(&m_offset);
		}
	}

	context.read(&m_length);

	m_hash.read(context);

	bool has_recipe;
	if (in_chunk_table)
	{
		has_recipe = true;
	}
	else
	{
		context.read(&has_recipe);
	}

	context.m_current_item_blobdef.m_length = m_length;
	context.m_current_item_blobdef.m_hashes.clear();
	context.m_current_item_blobdef.m_hashes.push_back(m_hash);

	if (has_recipe)
	{
		uint32_t recipe_type = recipe::read_recipe_type(context);

		auto recipe_host = context.get_recipe_host();

		m_recipe = recipe_host->create_recipe(recipe_type, context.m_current_item_blobdef);

		m_recipe->read(context);
	}
}

void diffs::archive_item::write(diff_writer_context &context, bool in_chunk_table)
{
	if (in_chunk_table)
	{
		assert(m_type == archive_item_type::chunk);
	}
	else
	{
		context.write(static_cast<uint8_t>(m_type));
		if (m_type == archive_item_type::chunk)
		{
			context.write(m_offset);
		}
	}

	context.write(m_length);
	m_hash.write(context);

	if (in_chunk_table)
	{
		assert(m_recipe.get() != nullptr);
	}

	if (m_recipe.get() == nullptr)
	{
		context.write(false);
	}
	else
	{
		if (!in_chunk_table)
		{
			context.write(true);
		}

		recipe::write_recipe_type(context, *m_recipe.get());

		m_recipe->write(context);
	}
}

void diffs::archive_item::apply(diffs::apply_context &context) const
{
	if (m_length == 0)
	{
		return;
	}

	if (m_recipe.get() == nullptr)
	{
		throw error_utility::user_exception(error_utility::error_code::diff_archive_item_missing_recipe);
	}

	apply_context recipe_context = apply_context::child_context(&context, m_offset, m_length);
	m_recipe->apply(recipe_context);

	auto actual = recipe_context.get_target_hash();
	hash::verify_hashes_match(actual, m_hash);
}

std::unique_ptr<io_utility::reader> diffs::archive_item::make_reader(apply_context &context) const
{
	apply_context recipe_context = apply_context::child_context(&context, m_offset, m_length);
	return has_recipe() ? m_recipe->make_reader(recipe_context) : std::unique_ptr<io_utility::reader>{};
}

uint64_t diffs::archive_item::get_inline_asset_byte_count() const
{
	if (m_recipe.get() == nullptr)
	{
		return 0;
	}

	return m_recipe->get_inline_asset_byte_count();
}

void diffs::archive_item::prep_blob_cache(diffs::apply_context &context) const
{
	if (m_recipe.get() == nullptr)
	{
		return;
	}

	m_recipe->prep_blob_cache(context);
}

diffs::recipe *diffs::archive_item::create_recipe(const recipe_host *recipe_host, const char *recipe_type_name)
{
	blob_definition blobdef;
	blobdef.m_length = m_length;
	blobdef.m_hashes.push_back(m_hash);

	m_recipe = recipe_host->create_recipe(recipe_type_name, blobdef);

	return m_recipe.get();
}
