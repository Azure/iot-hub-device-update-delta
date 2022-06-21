/**
 * @file recipe.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <vector>
#include <memory>

#include "hash.h"
#include "recipe_parameter.h"
#include "recipe_type.h"
#include "archive_item_type.h"

#include "apply_context.h"

namespace diffs
{
class recipe_parameter;
class diff;

class recipe
{
	public:
	recipe(recipe_type type, const blob_definition &blobdef) : m_type(type), m_blobdef(blobdef) {}

	void write(diff_writer_context &context);

	// void verify(diff_viewer_context &context) const;

	uint64_t get_inline_asset_byte_count() const;

	recipe_type get_type() const { return m_type; }
	const std::vector<recipe_parameter> &get_parameters() const { return m_parameters; }

	recipe(recipe_type type) : m_type(type) {}

	virtual void apply(apply_context &context) const = 0;

	void add_parameter(recipe_parameter &&parameter);

	recipe_parameter *add_archive_item_parameter(
		diffs::archive_item_type type,
		uint64_t offset,
		uint64_t length,
		hash_type hash_type,
		const char *hash_value,
		size_t hash_value_length);

	void add_number_parameter(uint64_t number);

	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const;

	virtual void prep_blob_cache(diffs::apply_context &context) const;

	protected:
	void verify_parameter_count(size_t count) const;

	recipe_type m_type{};
	std::vector<recipe_parameter> m_parameters;
	blob_definition m_blobdef;
};
} // namespace diffs