/**
 * @file builtin_recipe_types.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <diffs/core/archive.h>

namespace archive_diff::diffs::serialization::standard
{
void ensure_builtin_recipe_types(core::archive *archive);
}