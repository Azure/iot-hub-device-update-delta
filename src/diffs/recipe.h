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
#include "archive_item_type.h"

#include "apply_context.h"

namespace diffs
{
class recipe_parameter;

class recipe
{
	public:
	recipe() : m_blobdef(blob_definition{}) {}
	recipe(const blob_definition &blobdef) : m_blobdef(blobdef) {}

	static void write_recipe_type(diff_writer_context &context, const recipe &recipe);
	static uint32_t read_recipe_type(diff_reader_context &context);

	virtual void write(diff_writer_context &context);
	virtual void read(diff_reader_context &context);

	void read_parameters(diff_reader_context &context);

	// void verify(diff_viewer_context &context) const;

	virtual std::string get_recipe_type_name() const                             = 0;
	virtual std::unique_ptr<recipe> create(const blob_definition &blobdef) const = 0;

	virtual uint64_t get_inline_asset_byte_count() const;

	const std::vector<recipe_parameter> &get_parameters() const { return m_parameters; }

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

	std::vector<recipe_parameter> m_parameters;
	blob_definition m_blobdef;
};

template <typename T>
class recipe_base : public recipe
{
	public:
	using recipe_base_type = recipe_base<T>;

	recipe_base() = default;
	recipe_base(const blob_definition &blobdef) : recipe(blobdef) {}

	virtual std::unique_ptr<recipe> create(const blob_definition &blobdef) const
	{
		return std::make_unique<T>(blobdef);
	}
};
} // namespace diffs