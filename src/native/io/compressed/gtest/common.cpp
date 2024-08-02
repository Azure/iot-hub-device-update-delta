/**
 * @file common.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <limits>

#include <io/reader.h>

std::vector<char> reader_to_vector(archive_diff::io::reader &reader)
{
	auto data_size = reader.size();
	if (data_size > std::numeric_limits<size_t>::max())
	{
		throw std::exception();
	}

	std::vector<char> data_vector;
	data_vector.reserve(static_cast<size_t>(data_size));

	auto data = std::span<char>{data_vector.data(), data_vector.capacity()};
	reader.read(0, data);

	return data_vector;
}