/**
 * @file aduapi_type_conversion.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduapi_type_conversion.h"

archive_diff::hashing::algorithm diffc_hash_type_to_algorithm(uint32_t type)
{
	switch (static_cast<diffc_hash_type>(type))
	{
	case diffc_hash_type::diffc_hash_md5:
		return archive_diff::hashing::algorithm::md5;
	case diffc_hash_type::diffc_hash_sha256:
		return archive_diff::hashing::algorithm::sha256;
	}
	std::string msg = "Unexpected hash type: " + std::to_string(static_cast<int>(type));
	throw archive_diff::errors::user_exception(archive_diff::errors::error_code::api_unexpected_hash_type, msg);
}

archive_diff::diffs::core::item_definition diffc_item_definition_to_core_item_definition(
	const diffc_item_definition &diffc_item)
{
	auto item = archive_diff::diffs::core::item_definition{diffc_item.m_length};

	for (size_t i = 0; i < diffc_item.m_hashes_count; i++)
	{
		auto &diffc_hash = diffc_item.m_hashes[i];

		auto algorithm = diffc_hash_type_to_algorithm(diffc_hash->m_hash_type);
		auto hash      = archive_diff::hashing::hash::import_hash_value(
            algorithm, std::string_view(diffc_hash->m_hash_value, diffc_hash->m_hash_length));

		item = item.with_hash(hash);
	}

	for (size_t i = 0; i < diffc_item.m_names_count; i++)
	{
		auto diffc_name = diffc_item.m_names[i];

		item = item.with_name(diffc_name);
	}

	return item;
}