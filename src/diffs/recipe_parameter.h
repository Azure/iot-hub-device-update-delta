/**
 * @file recipe_parameter.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "apply_context.h"
#include "user_exception.h"

namespace diffs
{
enum class recipe_parameter_type : uint8_t
{
	archive_item = 0,
	number       = 1,
};

const std::string RECIPE_PARAMETER_TYPE_STRINGS[] = {"archive_item", "recipe", "number", "inline_asset"};
inline bool is_valid_recipe_parameter_type(recipe_parameter_type value)
{
	return (value >= recipe_parameter_type::archive_item) && (value <= recipe_parameter_type::number);
}
inline const std::string &get_recipe_parameter_type_string(recipe_parameter_type type)
{
	if (!is_valid_recipe_parameter_type(type))
	{
		std::string msg =
			"diffs::get_recipe_parameter_type_string(): Invalid "
			"recipe parameter type: "
			+ std::to_string(static_cast<int>(type));
		throw error_utility::user_exception(error_utility::error_code::diff_invalid_recipe_parameter_type, msg);
	}

	return RECIPE_PARAMETER_TYPE_STRINGS[(int)type];
}

class recipe;
class diff;
class archive_item;
class recipe_parameter
{
	public:
	recipe_parameter() = default;
	recipe_parameter(std::uint64_t number);
	recipe_parameter(archive_item &&item);

	recipe_parameter(recipe_parameter &&) noexcept = default;
	~recipe_parameter();

	//	void dump(diff_viewer_context &context, std::ostream &ostream, const std::string &indent) const;
	void read(diff_reader_context &context);
	void write(diff_writer_context &context);
	uint64_t get_inline_asset_byte_count() const;
	uint64_t get_number_value() const;
	const archive_item *get_archive_item_value() const { return m_archive_item_value.get(); }
	archive_item *get_archive_item_value() { return m_archive_item_value.get(); }

	void apply(diffs::apply_context &context) const;
	recipe_parameter_type get_type() const { return m_type; }

	private:
	recipe_parameter_type m_type{};
	uint64_t m_number_value{};
	std::unique_ptr<archive_item> m_archive_item_value;
};
} // namespace diffs
