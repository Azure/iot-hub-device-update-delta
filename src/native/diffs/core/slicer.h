/**
 * @file slicer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <atomic>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "item_definition.h"
#include "prepared_item.h"

namespace archive_diff::diffs::core
{
class slicer
{
	public:
	slicer() = default;
	virtual ~slicer();

	public:
	// starts a thread using the item specified
	void slice_and_populate_slice_store(const item_definition &item);

	void request_slice(std::shared_ptr<prepared_item> &to_slice, uint64_t offset, const item_definition &slice);

	// Fetches items from slice store and decrements count
	std::shared_ptr<prepared_item> fetch_slice(const item_definition &item);

	// transition from paused -> running
	// takes m_item_to_slice_prepared and processes it
	// blocks until there are no paused threads
	void resume_slicing();

	// transition from running -> paused
	// signals to running threads to pause
	// blocks until there are no running threads
	void pause_slicing();

	// transitions from any state to cancelled
	// signals any running threads that may be paused
	// so they check to see if cancellation is requested
	void cancel_slicing();

	private:
	bool try_consume_slice(const item_definition &item, std::shared_ptr<prepared_item> *result);

	enum class slicing_state
	{
		paused,
		running,
		cancelled,
	};

	slicing_state m_state{slicing_state::paused};

	using offset_to_item_map = std::map<uint64_t, item_definition>;
	std::map<item_definition, std::unique_ptr<offset_to_item_map>> m_items_to_slices_requested;
	std::map<item_definition, std::shared_ptr<prepared_item>> m_item_to_slice_prepared;

	std::map<item_definition, std::pair<std::shared_ptr<prepared_item>, uint32_t>> m_stored_slices;

	using slice_item_to_request_count_map = std::map<item_definition, uint32_t>;
	slice_item_to_request_count_map m_slice_item_request_counts;

	std::atomic<uint32_t> m_starting_thread_count{0};
	std::atomic<uint32_t> m_paused_thread_count{0};
	std::atomic<uint32_t> m_running_thread_count{0};

	std::mutex m_request_mutex;
	std::mutex m_item_to_slice_prepared_mutex;
	std::mutex m_state_mutex;
	std::mutex m_store_mutex;
	std::condition_variable_any m_state_cv;
	std::condition_variable m_store_cv;

	std::vector<std::thread> m_slicing_threads;
};
} // namespace archive_diff::diffs::core