/**
 * @file blob_cache.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "hash.h"
#include "reader.h"
#include "reader_factory.h"
#include "hash_utility.h"

#include <atomic>
#include <vector>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <condition_variable>

namespace diffs
{
struct blob_definition
{
	public:
	uint64_t m_length{};
	std::vector<hash> m_hashes;
};

bool operator<(const blob_definition &left, const blob_definition &right);

class blob_cache
{
	public:
	void add_blob_source(io_utility::reader *reader, uint64_t offset, const blob_definition &blobdef);
	bool has_blob_source(io_utility::reader *reader, const blob_definition blobdef);

	void add_blob_request(const blob_definition &definition);
	void remove_blob_request(const blob_definition &blobdef);
	std::unique_ptr<io_utility::reader> get_blob_reader(const blob_definition &blobdef);
	bool is_blob_reader_available(const blob_definition &blobdef);

	std::unique_ptr<io_utility::reader> wait_for_reader(const blob_definition &blobdef);

	uint32_t get_blob_request_count(const blob_definition &blobdef);
	void populate_from_reader(io_utility::reader *source_reader);

	using blob_location_map = std::multimap<uint64_t, blob_definition>;
	blob_location_map get_blob_locations_for_reader(io_utility::reader *source_reader);
	blob_location_map take_blob_locations_for_reader(io_utility::reader *source_reader);
	void clear_blob_locations_for_reader(io_utility::reader *source_reader);

	void cancel() { m_cancel = true; }

	private:
	uint32_t get_blob_request_count_impl(const blob_definition &blobdef);
	bool is_blob_reader_available_impl(const blob_definition &blobdef);
	std::unique_ptr<io_utility::reader> get_blob_reader_impl(const blob_definition &blobdef);

	std::map<io_utility::reader *, std::set<blob_definition>> m_reader_blobdefs;
	std::map<io_utility::reader *, blob_location_map> m_reader_blob_location_map;

	std::map<blob_definition, std::unique_ptr<io_utility::reader_factory>> m_reader_factory_cache;
	std::map<blob_definition, uint32_t> m_blob_requests;

	std::mutex m_request_mutex;
	std::mutex m_reader_cache_mutex;

	std::condition_variable m_reader_cache_cv;

	std::atomic<bool> m_cancel{false};
};
} // namespace diffs
