/**
 * @file verified_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "reader.h"

#include <cinttypes>

#include "hash.h"
#include "hash_utility.h"

namespace diffs
{
class verified_reader : public io_utility::reader
{
	public:
	verified_reader(uint64_t chunk_offset, io_utility::unique_reader &&reader, hash hash_value) :
		m_chunk_offset(chunk_offset), m_hash_value(hash_value), m_reader(std::move(reader))
	{}

	virtual read_style get_read_style() const { return m_reader->get_read_style(); }

	virtual uint64_t size() { return m_reader->size(); }

	protected:
	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer)
	{
		auto actual_read = m_reader->read_some(offset, buffer);

		if (m_hashing)
		{
			if (offset == 0)
			{
				printf("Reading from reader for chunk at: %" PRIu64 "\n", m_chunk_offset);
			}

			if (m_end_of_last_read == offset)
			{
				m_hasher.hash_data(std::string_view{buffer.data(), actual_read});

				m_end_of_last_read = offset + actual_read;

				if (m_end_of_last_read == size())
				{
					auto hash     = m_hasher.get_hash_binary();
					auto actual   = std::string_view{hash.data(), hash.size()};
					auto expected = std::string_view{m_hash_value.m_hash_data.data(), m_hash_value.m_hash_data.size()};

					hash::verify_hashes_match(actual, expected);
					printf("Verified reader.\n");
				}
			}
			else
			{
				m_hashing = false;
			}
		}

		return actual_read;
	}

	private:
	uint64_t m_chunk_offset{};
	io_utility::unique_reader m_reader;
	hash m_hash_value;
	hash_utility::hasher m_hasher{hash_utility::algorithm::SHA256};

	uint64_t m_end_of_last_read{0};
	bool m_hashing{true};
};
} // namespace diffs
