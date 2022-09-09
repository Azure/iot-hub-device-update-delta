/**
 * @file diff_writer_context.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <map>

#include "reader.h"
#include "writer.h"

#include "wrapped_writer_sequential_writer.h"

#include "recipe_host.h"

namespace diffs
{
class diff_writer_context
{
	public:
	diff_writer_context(
		io_utility::writer *diff_writer,
		io_utility::reader *inline_asset_reader,
		io_utility::reader *remainder_reader) :
		m_diff_writer(diff_writer),
		m_diff_sequential_writer(diff_writer), m_inline_asset_reader(inline_asset_reader),
		m_remainder_reader(remainder_reader)
	{}
	void write_raw(std::string_view buffer);

	template <typename T>
	void write_raw(T value)
	{
		write_raw(std::string_view{reinterpret_cast<char *>(&value), sizeof(T)});
	}

	void write(std::string_view buffer) { write_raw(buffer); }

	template <typename T>
	void write(T value)
	{
		write(std::string_view{reinterpret_cast<char *>(&value), sizeof(T)});
	}

	void write_inline_assets();
	void write_remainder();

	uint64_t get_inline_asset_byte_count() { return m_inline_asset_reader->size(); }

	void set_recipe_host(recipe_host *recipe_host) { m_recipe_host = recipe_host; }
	recipe_host *get_recipe_host() const { return m_recipe_host; }

	private:
	recipe_host *m_recipe_host{};

	std::map<std::string, uint32_t> m_recipe_type_index_map;
	io_utility::writer *m_diff_writer{};
	io_utility::wrapped_writer_sequential_writer m_diff_sequential_writer;
	io_utility::reader *m_inline_asset_reader{};
	io_utility::reader *m_remainder_reader{};
};
} // namespace diffs