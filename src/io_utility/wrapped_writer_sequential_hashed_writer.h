/**
 * @file wrapped_writer_sequential_hashed_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "hashed_writer.h"
#include "wrapped_writer_sequential_writer.h"

namespace io_utility
{
class wrapped_writer_sequential_hashed_writer : public hashed_writer
{
	public:
	wrapped_writer_sequential_hashed_writer(writer *writer, hash_utility::hasher *hasher) : hashed_writer(hasher)
	{
		m_writer = std::make_unique<wrapped_writer_sequential_writer>(writer);
	}

	virtual void flush() { m_writer->flush(); }

	protected:
	virtual void write_and_hash_impl(std::string_view buffer, std::vector<hash_utility::hasher *> &hashers);

	private:
	std::unique_ptr<wrapped_writer_sequential_writer> m_writer;
};
} // namespace io_utility