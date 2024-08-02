/**
 * @file aduapi_types.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
enum class diffc_hash_type : uint32_t
#else
typedef enum
#endif
{
	diffc_hash_md5    = 0,
	diffc_hash_sha256 = 1,
#ifdef __cplusplus
};
#else
} diffc_hash_type;
#endif

#pragma pack(push, 8)
struct diffc_hash
{
	uint32_t m_hash_type;
	const char *m_hash_value;
	size_t m_hash_length;
};
#pragma pack(pop)

#pragma pack(push, 8)
struct diffc_item_definition
{
	uint64_t m_length;
	diffc_hash **m_hashes;
	size_t m_hashes_count;
	char **m_names;
	size_t m_names_count;
};
#pragma pack(pop)
