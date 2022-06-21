/**
 * @file archive_item_type.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <cstdint>

namespace diffs
{
enum class archive_item_type : uint8_t
{
	blob    = 0,
	chunk   = 1,
	payload = 2,
};
}