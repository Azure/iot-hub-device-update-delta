/**
 * @file remainder_chunk_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "remainder_chunk_recipe.h"

#include "recipe_helpers.h"

void diffs::remainder_chunk_recipe::apply(apply_context &context) const
{
	verify_parameter_count(2);

	uint64_t remainder_chunk_length = m_parameters[RECIPE_PARAMETER_LENGTH].get_number_value();
	if (remainder_chunk_length > std::numeric_limits<size_t>::max())
	{
		throw error_utility::user_exception::value_exceeds_size_t(
			error_utility::error_code::diff_remainder_chunk_length_too_large,
			"Remainder chunk length too large",
			remainder_chunk_length);
	}

	auto remainder_reader       = context.get_remainder_reader();
	auto remainder_chunk_offset = remainder_reader->tellg();
	io_utility::child_reader reader(remainder_reader, remainder_chunk_offset, remainder_chunk_length);
	context.write_target(&reader);
}