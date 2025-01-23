/**
 * @file apply_session.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <diffs/core/kitchen.h>

#include "aduapi_types.h"
#include "session_base.h"

#ifndef CDECL
	#ifdef WIN32
		#define CDECL __cdecl
	#else
		#define CDECL
	#endif
#endif

namespace archive_diff::diffs::core
{
class archive;
}

namespace archive_diff::diffs::api
{
class apply_session : public session_base
{
	public:
	apply_session()  = default;
	~apply_session() = default;

	uint32_t add_archive(std::shared_ptr<diffs::core::archive> &archive);
	uint32_t add_archive(const std::string &path);
	uint32_t request_item(const core::item_definition &item);
	uint32_t add_file_to_pantry(const std::string &path);
	uint32_t clear_requested_items();
	uint32_t process_requested_items();
	uint32_t process_requested_items(
		bool select_recipes_only, const diffc_item_definition **mocked_items, size_t mocked_item_count);
	uint32_t resume_slicing();
	uint32_t cancel_slicing();
	uint32_t extract_item_to_path(const core::item_definition &item, const std::string &path);
	uint32_t save_selected_recipes(const std::string &path);

	private:
	std::mutex m_mutex;
	std::shared_ptr<core::kitchen> m_kitchen{core::kitchen::create()};
};
} // namespace archive_diff::diffs::api