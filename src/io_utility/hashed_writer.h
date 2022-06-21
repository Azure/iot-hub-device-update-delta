/**
 * @file hashed_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "sequential_writer.h"

#include "hash_utility.h"

namespace io_utility
{
class hashed_writer : public sequential_writer_impl
{
	public:
	hashed_writer(hash_utility::hasher *hasher) : m_hasher(hasher) {}
	virtual ~hashed_writer() = default;

	virtual void flush() = 0;

	void write_and_hash(std::string_view buffer, std::vector<hash_utility::hasher *> &hashers)
	{
		write_and_hash_impl(buffer, hashers);
		m_offset += buffer.size();
	}

	protected:
	virtual void write_and_hash_impl(std::string_view buffer, std::vector<hash_utility::hasher *> &hashers) = 0;
	virtual void write_impl(std::string_view buffer)
	{
		std::vector<hash_utility::hasher *> hashers;
		write_and_hash(buffer, hashers);
		//
		// We want to account for the value already added by sequential_writer::write(), so we deduct
		// here in the implementation and increment in the write_and_hash() as well.
		// This allows us to properly increment when we are called via write_and_hash() or
		// write().
		//
		m_offset -= buffer.size();
	}

	hash_utility::hasher *m_hasher{};
};

using unique_hashed_writer = std::unique_ptr<hashed_writer>;
} // namespace io_utility
