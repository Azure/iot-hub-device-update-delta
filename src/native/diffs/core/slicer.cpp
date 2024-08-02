/**
 * @file slicer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "slicer.h"
#include "prepared_item.h"

#include "adu_log.h"

#include <hashing/hasher.h>
#include <io/buffer/reader_factory.h>

#include <thread>
#include <limits>
#include <string>

namespace archive_diff::diffs::core
{
slicer::~slicer()
{
	cancel_slicing();
	for (auto &slicing_thread : m_slicing_threads)
	{
		slicing_thread.join();
	}
}

//////////////// Slicing Request Methods

static bool check_overlap(uint64_t offset1, uint64_t length1, uint64_t offset2, uint64_t length2)
{
	// we succeed if offset1 is not within [offset2, offset2 + length)
	// and offset2 is not within [offset1, offset1 + length1)
	bool offset1_check = (offset1 < offset2) || (offset1 >= (offset2 + length2));
	bool offset2_check = (offset2 < offset1) || (offset2 >= (offset1 + length1));

	return offset1_check && offset2_check;
}

void slicer::request_slice(std::shared_ptr<prepared_item> &to_slice, uint64_t offset, const item_definition &slice)
{
	// we shouldn't be trying to slice hashes out of a thing
	// when we don't know the hash and can't verify it
	if (!slice.has_any_hashes())
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_request_slice_no_hash, "Trying to hash item with no hash.");
	}

	// printf("slicer::request_slice() for: %s\n", slice.to_string().c_str());
	std::lock_guard<std::mutex> state_lock_guard{m_state_mutex};

	if (m_state != slicing_state::paused)
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_invalid_state,
			"slicer::request_slice: Expected slicing state to be paused, actual: " + std::to_string((int)m_state));
	}

	std::lock_guard<std::mutex> request_lock_guard{m_request_mutex};

	// Add an entry about the request we're making
	if (m_slice_item_request_counts.count(slice) != 0)
	{
		ADU_LOG("slicer::request_slice: Already have a request for this item: {}", slice.to_string());
		//  There's already a pending request for a matching slice
		//  make one more request and bail
		m_slice_item_request_counts[slice]++;
		return;
	}

	auto item_to_slice = to_slice->get_item_definition();

	if (m_item_to_slice_prepared.count(item_to_slice) == 0)
	{
		m_item_to_slice_prepared.insert(std::pair{item_to_slice, to_slice});
	}

	if (m_items_to_slices_requested.count(item_to_slice) == 0)
	{
		auto new_map = std::make_unique<offset_to_item_map>();
		new_map->insert(std::pair{offset, slice});
		m_items_to_slices_requested.insert(std::pair{item_to_slice, std::move(new_map)});
		m_slice_item_request_counts.insert(std::pair{slice, 1});
		return;
	}

	auto &offset_to_slice_map = *(m_items_to_slices_requested[item_to_slice].get());

	ADU_LOG("slicer::request_slice: Adding entry for offset: {}, for item: {}", offset, slice.to_string());

	// check that we're not overlapping with the previous result
	if (offset_to_slice_map.size() != 0)
	{
		// lower_bound is first offset that is not < offset, it is >= our offset
		auto lower_bound = offset_to_slice_map.lower_bound(offset);

		if (lower_bound != offset_to_slice_map.cend())
		{
			uint64_t lower_bound_offset = lower_bound->first;
			uint64_t lower_bound_length = lower_bound->second.size();

			if (!check_overlap(offset, slice.size(), lower_bound_offset, lower_bound_length))
			{
				std::string msg = "slicer::request_slice: Found overlap with slices. ";
				msg += "offset: " + std::to_string(offset);
				msg += ", length: " + std::to_string(slice.size());
				msg += ", lower_bound_offset: " + std::to_string(lower_bound_offset);
				msg += ", lower_bound_length: " + std::to_string(lower_bound_length);
				throw errors::user_exception(errors::error_code::diff_slicing_request_slice_overlap, msg);
			}
		}

		if (lower_bound != offset_to_slice_map.cbegin())
		{
			lower_bound--;

			uint64_t prev_offset = lower_bound->first;
			uint64_t prev_length = lower_bound->second.size();

			if (!check_overlap(offset, slice.size(), prev_offset, prev_length))
			{
				std::string msg = "slicer::request_slice: Found overlap with slices. ";
				msg += "offset: " + std::to_string(offset);
				msg += ", length: " + std::to_string(slice.size());
				msg += ", prev offset: " + std::to_string(prev_offset);
				msg += ", prev length: " + std::to_string(prev_length);
				throw errors::user_exception(errors::error_code::diff_slicing_request_slice_overlap, msg);
			}
		}
	}

	m_slice_item_request_counts.insert(std::pair{slice, 1});
	offset_to_slice_map.insert(std::pair{offset, slice});
}

bool slicer::try_consume_slice(const item_definition &item, std::shared_ptr<prepared_item> *result)
{
	std::lock_guard<std::mutex> lock(m_store_mutex);

	if (m_state != slicing_state::running)
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_invalid_state,
			"slicer::try_consume_slice: Expected slicing state to be running, actual: " + std::to_string((int)m_state));
	}

	if (m_stored_slices.count(item) == 0)
	{
		return false;
	}

	auto stored_slice = m_stored_slices[item];

	auto &count = stored_slice.second;

	// should not be possible, exception
	if (count == 0)
	{
		throw errors::user_exception(errors::error_code::diff_slicing_no_stored_item, "Zero stored slices for item.");
	}

	count--;
	// we no longer need this, throw it away
	if (count == 0)
	{
		result->swap(stored_slice.first);
		m_stored_slices.erase(item);
	}
	else
	{
		*result = stored_slice.first;
	}

	return true;
}

// Fetches items from slice store and decrements count. Waits if necessary.
std::shared_ptr<prepared_item> slicer::fetch_slice(const item_definition &item)
{
	if (m_state != slicing_state::running)
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_invalid_state,
			"slicer::fetch_slice: Expected slicing state to be running, actual: " + std::to_string((int)m_state));
	}

	// printf("Trying to fetch slice: %s\n", item.to_string().c_str());

	std::shared_ptr<prepared_item> result;
	while (true)
	{
		if (try_consume_slice(item, &result))
		{
			break;
		}

		// not currently available, wait for it
		// printf("Waiting for %s\n", item.to_string().c_str());
		std::unique_lock<std::mutex> lock(m_store_mutex);
		m_store_cv.wait(lock, [&] { return (m_stored_slices.count(item) > 0); });
	}

	return result;
}

struct slicing_thread_data
{
	item_definition m_item;
	slicer *m_slicer;
};

static void slicing_thread_worker(slicing_thread_data data)
{
	// printf("In slicing_thread_worker\n");
	data.m_slicer->slice_and_populate_slice_store(data.m_item);
}

// transition from paused -> running
// takes m_pending_items_to_slice and processes it
// blocks until there are no paused threads
void slicer::resume_slicing()
{
	if (m_state == slicing_state::cancelled)
	{
		return;
	}

	{
		// No one can modify the requests now
		std::lock_guard<std::mutex> requests_lock_guard{m_request_mutex};

		{
			std::lock_guard<std::mutex> state_lock_guard{m_state_mutex};
			m_state = slicing_state::running;
		}

		for (auto &itr : m_item_to_slice_prepared)
		{
			m_starting_thread_count++;
			// printf("Kicking off thread to slice: %s\n", itr.first.to_string().c_str());
			slicing_thread_data thread_data{itr.first, this};
			m_slicing_threads.emplace_back(std::thread{slicing_thread_worker, thread_data});
		}

		// printf("Waiting for there to be no pending slicing threads.\n");
		//   wait until there are no more slicing threads starting up

		{
			std::unique_lock<std::mutex> lock(m_state_mutex);
			m_state_cv.wait(
				lock, [&] { return (m_starting_thread_count == 0) || (m_state == slicing_state::cancelled); });
		}

		// we're done starting up, we can clear this as the working threads
		// have already used this
		m_item_to_slice_prepared.clear();
	}

	// new requests are now permitted. We may need them to build the reader
	// for an item we're slicing.
	if (m_state == slicing_state::cancelled)
	{
		return;
	}

	{
		std::unique_lock<std::mutex> lock(m_state_mutex);
		m_state_cv.wait(lock, [&] { return (m_paused_thread_count == 0); });
	}
	// printf("No more pending slicing threads.\n");
}

// transition from running -> paused
// signals to running threads to pause
// blocks until there are no running threads
void slicer::pause_slicing()
{
	if (m_state == slicing_state::cancelled || m_state == slicing_state::paused)
	{
		return;
	}

	m_state = slicing_state::paused;

	// printf("Waiting for there to be no running slincing threads.\n");
	//   wait until there are no more slicing threads running
	std::unique_lock<std::mutex> lock(m_state_mutex);
	m_state_cv.wait(lock, [&] { return m_running_thread_count == 0 || m_state == slicing_state::cancelled; });
}

// m_request_mutex should be held should be held on main thread until we're done starting
// We will notify the main thread we are done starting when we have consumed
// an entry from m_items_to_slices_requested
void slicer::slice_and_populate_slice_store(const item_definition &item_to_slice)
{
	// printf("Slicing and populating using item: %s\n", item_to_slice.to_string().c_str());
	if (m_state != slicing_state::running)
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_invalid_state,
			"slicer::slice_and_populate_slice_store: Expected slicing state to be running, actual: "
				+ std::to_string((int)m_state));
	}

	if (m_items_to_slices_requested.count(item_to_slice) == 0)
	{
		throw errors::user_exception(
			errors::error_code::diff_slicing_no_slices_requested,
			"slicer::slice_and_populate_slice_store: No slices requested");
	}

	std::unique_ptr<offset_to_item_map> slices_requested;
	slices_requested.swap(m_items_to_slices_requested[item_to_slice]);
	m_items_to_slices_requested.erase(item_to_slice);

	std::shared_ptr<prepared_item> prepared_item_to_slice = m_item_to_slice_prepared[item_to_slice];

	m_running_thread_count++;
	m_starting_thread_count--;
	m_state_cv.notify_all();

	// this may end up kicking off more prepares and slicing needed
	auto reader = prepared_item_to_slice->make_sequential_reader();

	// printf("We're slicing %s\n", item_to_slice.to_string().c_str());
	hashing::hasher hasher(hashing::algorithm::sha256);

	for (auto offset_and_slice : *slices_requested)
	{
		if (m_state == slicing_state::paused)
		{
			m_paused_thread_count++;
			m_running_thread_count--;
			m_state_cv.notify_all();
			std::unique_lock<std::mutex> lock(m_state_mutex);
			m_state_cv.wait(lock, [&] { return (m_state != slicing_state::paused); });
			m_running_thread_count++;
			m_paused_thread_count--;
		}

		if (m_state == slicing_state::cancelled)
		{
			return;
		}

		auto offset = offset_and_slice.first;
		auto slice  = offset_and_slice.second;

		ADU_LOG(
			"slicer::slice_and_populate_slice_store: Slicing {} out of {}",
			slice.to_string(),
			item_to_slice.to_string());

		auto current_offset = reader->tellg();

		ADU_LOG("Expecting slice: {} at offset: {}. Current offset: {}", slice.to_string(), offset, current_offset);

		if (current_offset > offset)
		{
			std::string msg = "slicer::request_slice: Found overlap with slices. ";
			msg += "offset: " + std::to_string(offset);
			msg += ", current: " + std::to_string(current_offset);
			throw errors::user_exception(errors::error_code::diff_slicing_request_slice_overlap, msg);
		}

		auto to_skip = offset - current_offset;
		if (to_skip)
		{
			ADU_LOG(
				"slicer::slice_and_populate_slice_store: Skipped {} in reader of {} bytes.", to_skip, reader->size());
			reader->skip(to_skip);
		}

		auto slice_vector = std::make_shared<std::vector<char>>();
		if (slice.size() > std::numeric_limits<size_t>::max())
		{
			throw errors::user_exception(errors::error_code::diff_slicing_request_size_too_large);
		}
		slice_vector->reserve(static_cast<size_t>(slice.size()));

		reader->read(std::span<char>(slice_vector->data(), slice_vector->capacity()));
		ADU_LOG(
			"slicer::slice_and_populate_slice_store: Read {} in reader of {} bytes.",
			slice_vector->capacity(),
			reader->size());

		hasher.reset();
		hasher.hash_data(std::string_view{slice_vector->data(), slice_vector->capacity()});
		auto slice_hash = hasher.get_hash();

		if (!slice.has_matching_hash(slice_hash))
		{
			std::string msg = "Generated slice doesn't match expected slice. ";
			msg += "Current offset: " + std::to_string(offset);
			msg += ", Slice: " + slice.to_string();
			msg += ", slice_hash: " + slice_hash.get_string();
			throw errors::user_exception(errors::error_code::diff_slicing_produced_hash_mismatch, msg);
		}

		std::shared_ptr<io::reader_factory> cache_entry_factory = std::make_shared<io::buffer::reader_factory>(
			slice_vector, io::buffer::io_device::size_kind::vector_capacity);

		auto prep_slice = std::make_shared<diffs::core::prepared_item>(
			slice, diffs::core::prepared_item::reader_kind{cache_entry_factory});

		{
			std::lock_guard<std::mutex> lock(m_store_mutex);

			if (m_slice_item_request_counts.count(slice) == 0)
			{
				throw errors::user_exception(
					errors::error_code::diff_slicing_no_requests_for_slice, "No requests for slice.");
			}

			auto count = m_slice_item_request_counts[slice];

			ADU_LOG("Stored slice for {}.", slice.to_string());
			m_stored_slices.insert(std::pair{slice, std::pair{prep_slice, count}});

			m_slice_item_request_counts.erase(slice);
		}

		ADU_LOG("Notified: m_stored_slices_cv for {}", slice.to_string());
		m_store_cv.notify_all();
	}
}

void slicer::cancel_slicing()
{
	{
		std::lock_guard<std::mutex> lock_guard{m_state_mutex};
		m_state = slicing_state::cancelled;
	}
	m_state_cv.notify_all();
}

} // namespace archive_diff::diffs::core