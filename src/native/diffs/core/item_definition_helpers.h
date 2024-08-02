/**
 * @file item_definition_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <string_view>

#include <span>

#include <io/reader.h>

#include "item_definition.h"

namespace archive_diff::diffs::core
{
item_definition create_definition_from_span(const std::span<char> data);
item_definition create_definition_from_string_view(std::string_view data);
item_definition create_definition_from_vector_using_capacity(std::vector<char> &data);
item_definition create_definition_from_vector_using_size(std::vector<char> &data);
item_definition create_definition_from_reader(io::reader &reader);
item_definition create_definition_from_sequential_reader(io::sequential::reader *reader);
} // namespace archive_diff::diffs::core