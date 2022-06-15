/**
 * @file blob_cache.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "blob_cache.h"
#include "hash_utility.h"

#include "stored_blob_reader_factory.h"

bool diffs::operator<(const blob_definition &left, const blob_definition &right)
{
	if (left.m_length != right.m_length)
	{
		return left.m_length < right.m_length;
	}

	if (left.m_hashes.size() != right.m_hashes.size())
	{
		return left.m_hashes.size() < right.m_hashes.size();
	}

	for (size_t i = 0; i < left.m_hashes.size(); i++)
	{
		auto &left_hash  = left.m_hashes[i];
		auto &right_hash = right.m_hashes[i];

		if (left_hash.m_hash_type != right_hash.m_hash_type)
		{
			return left_hash.m_hash_type < right_hash.m_hash_type;
		}

		auto left_hash_size  = left_hash.m_hash_data.size();
		auto right_hash_size = right_hash.m_hash_data.size();

		if (left_hash_size != right_hash_size)
		{
			return left_hash_size < right_hash_size;
		}

		if (memcmp(left_hash.m_hash_data.data(), right_hash.m_hash_data.data(), left_hash_size) < 0)
		{
			return true;
		}
	}

	return false;
}

void diffs::blob_cache::add_blob_source(io_utility::reader *reader, uint64_t offset, const blob_definition &blobdef)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	if (m_reader_blob_location_map.count(reader))
	{
		m_reader_blobdefs.emplace(reader, std::set<blob_definition>{});
		m_reader_blob_location_map.emplace(reader, blob_location_map{});
	}

	auto count = m_reader_blobdefs[reader].count(blobdef);

	// don't add duplicate entry.
	if (count == 1)
	{
		return;
	}

	m_reader_blobdefs[reader].insert(blobdef);
	m_reader_blob_location_map[reader].emplace(std::make_pair(offset, blobdef));
}

bool diffs::blob_cache::has_blob_source(io_utility::reader *reader, const blob_definition blobdef)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	if (m_reader_blobdefs.count(reader) == 0)
	{
		return false;
	}

	return (m_reader_blobdefs[reader].count(blobdef) > 0);
}

void diffs::blob_cache::add_blob_request(const blob_definition &definition)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	auto requests               = get_blob_request_count_impl(definition);
	m_blob_requests[definition] = requests + 1;
}

void diffs::blob_cache::remove_blob_request(const blob_definition &blobdef)
{
	std::lock_guard<std::mutex> reader_lock(m_request_mutex);
	std::lock_guard<std::mutex> cache_lock(m_reader_cache_mutex);

	auto requests = get_blob_request_count_impl(blobdef);

	if (requests == 0)
	{
		return;
	}

	if (requests == 1)
	{
		m_blob_requests.erase(blobdef);
		m_reader_factory_cache.erase(blobdef);
		return;
	}

	m_blob_requests[blobdef] = requests - 1;
}

std::unique_ptr<io_utility::reader> diffs::blob_cache::get_blob_reader(const blob_definition &blobdef)
{
	std::lock_guard<std::mutex> lock(m_reader_cache_mutex);

	return get_blob_reader_impl(blobdef);
}

std::unique_ptr<io_utility::reader> diffs::blob_cache::get_blob_reader_impl(const blob_definition &blobdef)
{
	if (!m_reader_factory_cache.count(blobdef))
	{
		return std::unique_ptr<io_utility::reader>{};
	}

	return m_reader_factory_cache[blobdef]->create();
}

bool diffs::blob_cache::is_blob_reader_available(const blob_definition &blobdef)
{
	std::lock_guard<std::mutex> lock(m_reader_cache_mutex);
	return is_blob_reader_available_impl(blobdef);
}

bool diffs::blob_cache::is_blob_reader_available_impl(const blob_definition &blobdef)
{
	return (m_reader_factory_cache.count(blobdef) > 0);
}

std::unique_ptr<io_utility::reader> diffs::blob_cache::wait_for_reader(const blob_definition &blobdef)
{
	auto count = get_blob_request_count(blobdef);

	if (count == 0)
	{
		return std::unique_ptr<io_utility::reader>{};
	}

	if (is_blob_reader_available(blobdef))
	{
		return get_blob_reader(blobdef);
	}

	std::unique_lock<std::mutex> lock(m_reader_cache_mutex);
	m_reader_cache_cv.wait(lock, [&] { return is_blob_reader_available_impl(blobdef); });

	return get_blob_reader_impl(blobdef);
}

uint32_t diffs::blob_cache::get_blob_request_count(const blob_definition &blobdef)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	uint32_t requests = 0;
	if (m_blob_requests.count(blobdef))
	{
		requests = m_blob_requests[blobdef];
	}

	return requests;
}

void diffs::blob_cache::populate_from_reader(io_utility::reader *source_reader)
{
	auto blob_locations = take_blob_locations_for_reader(source_reader);

	for (auto itr = blob_locations.begin(); itr != blob_locations.end(); itr++)
	{
		if (m_cancel)
		{
			return;
		}

		auto offset   = itr->first;
		auto length   = itr->second.m_length;
		auto &blobref = itr->second;

		auto new_factory = std::make_unique<io_utility::stored_blob_reader_factory>(source_reader, offset, length);

		std::lock_guard<std::mutex> lock(m_reader_cache_mutex);
		m_reader_factory_cache.emplace(std::make_pair(blobref, std::move(new_factory)));
		m_reader_cache_cv.notify_all();
	}
}

diffs::blob_cache::blob_location_map diffs::blob_cache::get_blob_locations_for_reader(io_utility::reader *source_reader)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	if (!m_reader_blob_location_map.count(source_reader))
	{
		return blob_location_map{};
	}

	return m_reader_blob_location_map[source_reader];
}

diffs::blob_cache::blob_location_map diffs::blob_cache::take_blob_locations_for_reader(
	io_utility::reader *source_reader)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	if (!m_reader_blob_location_map.count(source_reader))
	{
		return blob_location_map{};
	}

	auto locations = std::move(m_reader_blob_location_map[source_reader]);
	m_reader_blob_location_map.erase(source_reader);
	return locations;
}

void diffs::blob_cache::clear_blob_locations_for_reader(io_utility::reader *source_reader)
{
	std::lock_guard<std::mutex> lock(m_request_mutex);

	if (!m_reader_blob_location_map.count(source_reader))
	{
		return;
	}

	m_reader_blob_location_map.erase(source_reader);
}

uint32_t diffs::blob_cache::get_blob_request_count_impl(const blob_definition &blobdef)
{
	uint32_t requests = 0;
	if (m_blob_requests.count(blobdef))
	{
		requests = m_blob_requests[blobdef];
	}

	return requests;
}