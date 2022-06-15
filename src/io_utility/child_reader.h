/**
 * @file child_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <optional>

#include "reader.h"

namespace io_utility
{
class child_reader : public reader
{
	public:
	child_reader(reader *reader);
	child_reader(reader *reader, uint64_t base_offset, uint64_t length);

	static std::unique_ptr<child_reader> with_base_offset(reader *reader, uint64_t base_offset)
	{
		return std::make_unique<child_reader>(reader, base_offset, std::optional<uint64_t>());
	}

	static std::unique_ptr<child_reader> with_length(reader *reader, uint64_t length)
	{
		return std::make_unique<child_reader>(reader, std::optional<uint64_t>(), length);
	}

	child_reader(reader *reader, std::optional<uint64_t> base_offset, std::optional<uint64_t> length);

	virtual uint64_t size();

	protected:
	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer);

	virtual read_style get_read_style() const override;

	private:
	reader *m_parent_reader{};

	uint64_t m_base_offset{};
	std::optional<uint64_t> m_length;
};
} // namespace io_utility
