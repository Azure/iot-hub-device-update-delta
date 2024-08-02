/**
 * @file legacy_apply_session.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "legacy_apply_session.h"

#include <errors/user_exception.h>

#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
#include <io/basic_reader_factory.h>

#include <diffs/serialization/legacy/deserializer.h>
#include <diffs/serialization/standard/deserializer.h>

#include <diffs/core/archive.h>
#include <diffs/core/item_definition_helpers.h>

uint32_t archive_diff::diffs::api::apply_session::apply(
	const char *source_path, const char *diff_path, const char *target_path)
{
	clear_errors();

	try
	{
		auto diff_reader = archive_diff::io::file::io_device::make_reader(diff_path);

		auto kitchen = core::kitchen::create();

		std::shared_ptr<core::archive> archive;

		std::string reason_standard;
		if (archive_diff::diffs::serialization::standard::deserializer::is_this_format(diff_reader, &reason_standard))
		{
			archive_diff::diffs::serialization::standard::deserializer deserializer;
			deserializer.read(diff_reader);
			archive = deserializer.get_archive();
		}
		else
		{
			archive_diff::diffs::serialization::legacy::deserializer deserializer;
			deserializer.read(diff_reader);
			archive = deserializer.get_archive();
		}

		auto archive_item = archive->get_archive_item();
		archive->stock_kitchen(kitchen.get());

		auto source_reader = archive_diff::io::file::io_device::make_reader(source_path);
		auto source_item   = archive_diff::diffs::core::create_definition_from_reader(source_reader);
		std::shared_ptr<archive_diff::io::reader_factory> source_factory =
			std::make_shared<archive_diff::io::basic_reader_factory>(source_reader);
		auto source_prepped_item = std::make_shared<archive_diff::diffs::core::prepared_item>(
			source_item, archive_diff::diffs::core::prepared_item::reader_kind{source_factory});
		kitchen->store_item(source_prepped_item);

		archive_diff::io::file::binary_file_writer writer(target_path);

		kitchen->request_item(archive_item);
		if (!kitchen->process_requested_items())
		{
			throw std::exception();
		}

		kitchen->resume_slicing();
		kitchen->write_item(writer, archive_item);
		kitchen->cancel_slicing();

		return 0;
	}
	catch (std::exception &e)
	{
		std::string error_text = "Failed to apply diff. Exception: ";
		error_text += e.what();
		add_error(archive_diff::errors::error_code::standard_library_exception, error_text);
	}
	catch (archive_diff::errors::user_exception &e)
	{
		add_error(e.get_error(), e.get_message());
	}

	return get_error_count();
}
