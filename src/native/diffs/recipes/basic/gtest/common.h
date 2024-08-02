/**
 * @file common.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <diffs/core/item_definition.h>

using item_definition = archive_diff::diffs::core::item_definition;

const char c_alphabet[]            = "abcdefghijklmnopqrstuvwxyz";
const size_t c_letters_in_alphabet = sizeof(c_alphabet) - 1;

item_definition create_definition_from_data(std::vector<char> &data);
