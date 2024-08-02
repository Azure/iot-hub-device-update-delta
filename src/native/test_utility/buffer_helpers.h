/**
 * @file buffer_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <diffs/core/item_definition.h>

#include <vector>
#include <string_view>

archive_diff::diffs::core::item_definition create_definition_from_span(const std::span<char> data);
archive_diff::diffs::core::item_definition create_definition_from_data(std::string_view data);
archive_diff::diffs::core::item_definition create_definition_from_vector_using_capacity(std::vector<char> &data);

void modify_vector(
	std::vector<char> &data, size_t new_size, size_t set_to_four_ratio, size_t bitmask_ratio, size_t xor_ratio);
