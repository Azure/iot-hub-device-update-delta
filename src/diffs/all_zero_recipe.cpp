/**
 * @file all_zero_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "all_zero_recipe.h"
#include "archive_item.h"
#include "recipe_helpers.h"

#include <algorithm>
#include <cstring>

size_t diffs::zeroes_reader::read_some(uint64_t offset, gsl::span<char> buffer)
{
	auto available = std::max((uint64_t)0, m_length - offset);
	auto to_read   = std::min<size_t>(available, buffer.size());

	std::memset(buffer.data(), 0, to_read);
	return to_read;
}

void diffs::all_zero_recipe::apply(apply_context &context) const
{
	verify_parameter_count(1);

	auto &length_param = m_parameters[0];
	auto length        = convert_uint64_t<int>(
        length_param.get_number_value(),
        error_utility::error_code::diff_all_zero_length_too_large,
        "Region length too large");

	zeroes_reader reader(length);
	context.write_target(&reader);
}

std::unique_ptr<io_utility::reader> diffs::all_zero_recipe::make_reader(apply_context &context) const
{
	verify_parameter_count(1);

	auto &length_param = m_parameters[0];
	auto length        = convert_uint64_t<int>(
        length_param.get_number_value(),
        error_utility::error_code::diff_all_zero_length_too_large,
        "Region length too large");

	return std::make_unique<zeroes_reader>(length);
}
