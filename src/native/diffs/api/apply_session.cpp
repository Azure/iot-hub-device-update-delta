/**
 * @file apply_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "apply_session.h"

#include "aduapicall.h"

#include <diffs/core/archive.h>
#include <diffs/core/item_definition_helpers.h>
#include <diffs/serialization/legacy/deserializer.h>
#include <diffs/serialization/standard/deserializer.h>

#include <io/reader_factory.h>
#include <io/basic_reader_factory.h>
#include <io/file/binary_file_writer.h>
#include <io/file/io_device.h>

namespace archive_diff::diffs::api
{
uint32_t apply_session::add_archive(std::shared_ptr<diffs::core::archive> &archive)
{
	API_CALL_PROLOG();
	archive->stock_kitchen(m_kitchen.get());
	API_CALL_EPILOG();
}

uint32_t apply_session::add_archive(const std::string &path)
{
	std::shared_ptr<core::archive> archive;

	auto diff_reader = io::file::io_device::make_reader(path);

	API_CALL_PROLOG();

	std::string reason_standard;
	if (diffs::serialization::standard::deserializer::is_this_format(diff_reader, &reason_standard))
	{
		diffs::serialization::standard::deserializer deserializer;
		deserializer.read(diff_reader);
		archive = deserializer.get_archive();
	}
	else
	{
		ADU_LOG(
			"Using legacy deserializer, because archive was failed to load standard format. Reason: {}",
			reason_standard);

		diffs::serialization::legacy::deserializer deserializer;
		deserializer.read(diff_reader);
		archive = deserializer.get_archive();
	}

	add_archive(archive);
	API_CALL_EPILOG();
}

uint32_t apply_session::request_item(const core::item_definition &item)
{
	API_CALL_PROLOG();
	m_kitchen->request_item(item);
	API_CALL_EPILOG();
}

uint32_t apply_session::add_file_to_pantry(const std::string &path)
{
	API_CALL_PROLOG();

	auto reader = io::file::io_device::make_reader(path);

	auto item = diffs::core::create_definition_from_reader(reader);

	ADU_LOG("adding file to pantry: {}", item.to_string());

	std::shared_ptr<io::reader_factory> reader_factory = std::make_shared<io::basic_reader_factory>(reader);

	auto inline_assets_prep =
		std::make_shared<core::prepared_item>(item, diffs::core::prepared_item::reader_kind{reader_factory});

	m_kitchen->store_item(inline_assets_prep);

	API_CALL_EPILOG();
}

uint32_t apply_session::clear_requested_items()
{
	API_CALL_PROLOG();
	m_kitchen->clear_requested_items();
	API_CALL_EPILOG();
}

uint32_t apply_session::process_requested_items()
{
	API_CALL_PROLOG();
	if (!m_kitchen->process_requested_items())
	{
		return 1;
	}
	API_CALL_EPILOG();
}

uint32_t apply_session::resume_slicing()
{
	API_CALL_PROLOG();
	m_kitchen->resume_slicing();
	API_CALL_EPILOG();
}

uint32_t apply_session::cancel_slicing()
{
	API_CALL_PROLOG();
	m_kitchen->cancel_slicing();
	API_CALL_EPILOG();
}

uint32_t apply_session::extract_item_to_path(const core::item_definition &item, const std::string &path)
{
	API_CALL_PROLOG();

	auto prepared_item = m_kitchen->fetch_item(item);

	std::shared_ptr<io::writer> writer = std::make_shared<io::file::binary_file_writer>(path);
	prepared_item->write(writer);

	API_CALL_EPILOG();
}
} // namespace archive_diff::diffs::api