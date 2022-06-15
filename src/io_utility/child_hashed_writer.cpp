/**
 * @file child_hashed_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "user_exception.h"

#include "child_hashed_writer.h"

void io_utility::child_hashed_writer::flush() { m_parent->flush(); }

void io_utility::child_hashed_writer::write_and_hash_impl(
	std::string_view buffer, std::vector<hash_utility::hasher *> &hashers)
{
	if (m_parent == nullptr)
	{
		throw error_utility::user_exception(error_utility::error_code::io_child_reader_parent_is_null);
	}

	if (m_hasher != nullptr)
	{
		hashers.push_back(m_hasher);
	}

	m_parent->write_and_hash(buffer, hashers);
}