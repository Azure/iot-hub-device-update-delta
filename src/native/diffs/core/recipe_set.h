/**
 * @file recipe_set.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <set>

#include "recipe.h"

namespace archive_diff::diffs::core
{
using recipe_set        = std::set<std::shared_ptr<recipe>>;
using recipe_set_lookup = std::map<item_definition, recipe_set>;
} // namespace archive_diff::diffs::core