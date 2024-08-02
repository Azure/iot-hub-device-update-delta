/**
 * @file aduapi_type_conversion.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "aduapi_types.h"
#include <hashing/algorithm.h>
#include <diffs/core/item_definition.h>

archive_diff::hashing::algorithm diffc_hash_type_to_algorithm(uint32_t type);
archive_diff::diffs::core::item_definition diffc_item_definition_to_core_item_definition(
	const diffc_item_definition &diffc_item);