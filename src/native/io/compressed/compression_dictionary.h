/**
 * @file compression_dictionary.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string_view>
#include <io/reader.h>
#include <io/sequential/reader.h>

namespace archive_diff::io::compressed
{
// Doesn't own the data unless it we need to fetch from a reader
class compression_dictionary
{
	public:
	compression_dictionary() = default;

	compression_dictionary(std::shared_ptr<std::vector<char>> &data) : m_data(data), m_view(data->data(), data->size())
	{}

	compression_dictionary(io::reader &reader)
	{
		m_data = std::make_shared<std::vector<char>>();
		reader.read_all(*m_data.get());
		m_view = std::string_view{m_data->data(), m_data->size()};
	}

	compression_dictionary(io::sequential::reader &reader)
	{
		m_data = std::make_shared<std::vector<char>>();
		reader.read_all_remaining(*m_data.get());
		m_view = std::string_view{m_data->data(), m_data->size()};
	}

	std::string_view to_string_view() const { return m_view; }

	const char *data() const { return m_view.data(); }

	const size_t size() const { return m_view.size(); }

	private:
	std::shared_ptr<std::vector<char>> m_data;
	std::string_view m_view;
};
} // namespace archive_diff::io::compressed