/**
 * @file chained_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <gsl/span>

#include <vector>
#include <optional>

#include "reader.h"

namespace io_utility
{
class chained_reader : public reader
{
	public:
	chained_reader(gsl::span<reader *> readers);
	chained_reader(std::vector<unique_reader> &&readers);

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer);
	virtual read_style get_read_style() const override;
	virtual uint64_t size();

	private:
	void process_readers();
	reader *get_reader_for_offset(uint64_t offset);

	std::vector<reader *> m_readers;
	std::vector<unique_reader> m_readers_storage;

	std::optional<size_t> m_current_reader_index;
	uint64_t m_end_of_current_reader{};
	uint64_t m_start_of_current_reader{};

	uint64_t m_size{};
	read_style m_read_style{};
};
} // namespace io_utility