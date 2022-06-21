/**
 * @file child_hashed_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "hashed_writer.h"

namespace io_utility
{
class child_hashed_writer : public hashed_writer
{
	public:
	child_hashed_writer(hashed_writer *parent, hash_utility::hasher *hasher) : hashed_writer(hasher), m_parent(parent)
	{}

	virtual ~child_hashed_writer() = default;

	virtual void flush();

	protected:
	virtual void write_and_hash_impl(std::string_view buffer, std::vector<hash_utility::hasher *> &hashers);

	private:
	hashed_writer *m_parent{};
};
} // namespace io_utility