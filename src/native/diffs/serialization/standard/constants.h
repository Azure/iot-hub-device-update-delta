/**
 * @file constants.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

namespace archive_diff::diffs::serialization::standard
{
static const std::string g_DIFF_MAGIC_VALUE   = "PAMZ";
static const uint64_t g_STANDARD_DIFF_VERSION = 1;
static const uint64_t g_STANDARD_DIFF_VERSION_2 = 2;
} // namespace archive_diff::diffs::serialization::standard