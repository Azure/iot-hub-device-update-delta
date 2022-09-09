/**
 * @file recipe_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>
#include <string>

#include "error_codes.h"

#include "apply_context.h"
#include "archive_item.h"

#include "wrapped_readerwriter.h"
#include "temp_file_backed_reader.h"

#include "number_helpers.h"

#include "recipe.h"

#include "adu_log.h"

namespace diffs
{
const size_t RECIPE_PARAMETER_OFFSET = 0;
const size_t RECIPE_PARAMETER_LENGTH = 1;

const size_t RECIPE_PARAMETER_DELTA  = 0;
const size_t RECIPE_PARAMETER_SOURCE = 1;

const size_t RECIPE_PARAMETER_NESTED_OFFSET = 2;

template <typename T>
io_utility::unique_reader make_reader_using_apply(const T &object, apply_context &context)
{
	auto populate = [&](io_utility::readerwriter &readerwriter)
	{
		ADU_LOG("Populating from: %s", typeid(object).name());
		auto wrapped_readerwriter         = std::make_unique<io_utility::wrapped_readerwriter>(&readerwriter);
		diffs::apply_context file_context = diffs::apply_context::same_input_context(
			&context, std::move(wrapped_readerwriter), context.get_target_length());
		object.apply(file_context);
	};

	auto reader = std::make_unique<io_utility::temp_file_backed_reader>(populate);

	return reader;
}
} // namespace diffs