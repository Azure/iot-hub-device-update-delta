/**
 * @file extract.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string>

#include <language_support/include_filesystem.h>

#include <diffs/core/archive.h>
#include <diffs/core/item_definition_helpers.h>
#include <diffs/serialization/legacy/deserializer.h>
#include <diffs/serialization/standard/deserializer.h>

#include <io/reader_factory.h>
#include <io/basic_reader_factory.h>
#include <io/file/binary_file_writer.h>
#include <io/file/io_device.h>

#include <hashing/hexstring_convert.h>

using namespace archive_diff;

int try_extract(fs::path source_path, fs::path diff_path, std::string hash, size_t size, fs::path target_path);
int extract(fs::path source_path, fs::path diff_path, std::string hash, size_t size, fs::path target_path);

int main(int argc, char **argv)
{
	if (argc != 6)
	{
		printf("Usage: extract <source path> <diff path> <hash> <size> <target path>\n");
		return 1;
	}

	size_t size;
	std::string arg4(argv[4]);
	std::stringstream sstream(arg4);
	sstream >> size;

	return try_extract(argv[1], argv[2], argv[3], size, argv[5]);
}

std::shared_ptr<diffs::core::archive> load_diff(fs::path path)
{
	std::shared_ptr<diffs::core::archive> archive;

	auto diff_reader = io::file::io_device::make_reader(path.string());
	std::string reason_standard;
	if (diffs::serialization::standard::deserializer::is_this_format(diff_reader, &reason_standard))
	{
		diffs::serialization::standard::deserializer deserializer;
		deserializer.read(diff_reader);
		archive = deserializer.get_archive();
	}
	else
	{
		printf(
			"Using legacy deserializer, because archive was failed to load standard format. Reason: %s\n",
			reason_standard.c_str());

		diffs::serialization::legacy::deserializer deserializer;
		deserializer.read(diff_reader);
		archive = deserializer.get_archive();
	}

	return archive;
}

void add_file_to_pantry(diffs::core::kitchen &kitchen, fs::path path)
{
	auto reader = io::file::io_device::make_reader(path.string());
	auto item   = diffs::core::create_definition_from_reader(reader);

	printf("adding file to pantry: %s\n", item.to_string().c_str());

	std::shared_ptr<io::reader_factory> reader_factory = std::make_shared<io::basic_reader_factory>(reader);

	auto inline_assets_prep =
		std::make_shared<diffs::core::prepared_item>(item, diffs::core::prepared_item::reader_kind{reader_factory});

	kitchen.store_item(inline_assets_prep);
}

int try_extract(fs::path source_path, fs::path diff_path, std::string hash, size_t size, fs::path target_path)
{
	try
	{
		return extract(source_path, diff_path, hash, size, target_path);
	}
	catch (std::exception &e)
	{
		printf("Caught std::exception. Msg: %s", e.what());
		return 1;
	}
	catch (errors::user_exception &e)
	{
		printf("Caught errors::user_exception. Code: %d, Msg: %s\n", (int)e.get_error(), e.get_message());
		return 1;
	}

	return 0;
}

std::vector<diffs::core::item_definition> get_dependencies(
	diffs::core::archive &archive, const diffs::core::item_definition &item)
{
	std::vector<diffs::core::item_definition> dependencies;
	diffs::core::recipe_set const *recipes;

	if (!archive.get_cookbook()->find_recipes_for_item(item, &recipes))
	{
		printf("Couldn't find any recipe for item: %s\n", item.to_string().c_str());
		return {};
	}

	for (auto recipe : *recipes)
	{
		for (auto ingredient : recipe->get_item_ingredients())
		{
			dependencies.push_back(ingredient);
		}
	}

	return dependencies;
}

void extract_items(
	diffs::core::kitchen &kitchen,
	diffs::core::archive &archive,
	std::vector<diffs::core::item_definition> &items,
	fs::path target_path)
{
	std::map<diffs::core::item_definition, fs::path> item_paths;
	std::vector<diffs::core::item_definition> to_verify;

	kitchen.clear_requested_items();
	for (const auto &item : items)
	{
		if (!item.has_hash_for_alg(archive_diff::hashing::algorithm::sha256))
		{
			continue;
		}

		fs::path item_path;
		auto hashes = item.get_hashes();
		for (const auto &hash : hashes)
		{
			if (hash.first == archive_diff::hashing::algorithm::sha256)
			{
				item_path        = target_path / hash.second.get_data_string();
				item_paths[item] = item_path;
				break;
			}
		}

		if (fs::is_regular_file(item_path))
		{
			auto reader   = io::file::io_device::make_reader(item_path.string());
			auto new_item = diffs::core::create_definition_from_reader(reader);

			if (item.equals(new_item))
			{
				continue;
			}
		}

		kitchen.request_item(item);
	}

	if (!kitchen.process_requested_items())
	{
		printf("Failed to process requested items\n");
	}

	kitchen.resume_slicing();

	for (const auto &item : items)
	{
		if (!item_paths.contains(item))
		{
			continue;
		}

		auto prepared_item = kitchen.fetch_item(item);

		auto item_path                     = item_paths[item];
		std::shared_ptr<io::writer> writer = std::make_shared<io::file::binary_file_writer>(item_path.string());
		prepared_item->write(writer);
	}

	kitchen.cancel_slicing();

	for (const auto &item : items)
	{
		auto item_path = item_paths[item];
		auto reader    = io::file::io_device::make_reader(item_path.string());
		auto new_item  = diffs::core::create_definition_from_reader(reader);

		if (!item.equals(new_item))
		{
			printf(
				"Extracted item at %s is incorrect.\n\tExpected: %s\n\tActual: %s\n",
				item_path.string().c_str(),
				item.to_string().c_str(),
				new_item.to_string().c_str());
			auto deps = get_dependencies(archive, item);
			to_verify.insert(to_verify.end(), deps.begin(), deps.end());
		}
	}

	if (!to_verify.empty())
	{
		extract_items(kitchen, archive, to_verify, target_path);
	}
}

int extract(fs::path source_path, fs::path diff_path, std::string hash, size_t size, fs::path target_path)
{
	printf("Extract item with hash: %s\n", hash.c_str());
	printf("and length: %zu\n", size);
	printf("using source: %s\n", source_path.string().c_str());
	printf("and diff    :    %s\n", diff_path.string().c_str());
	printf("To folder   : %s\n", target_path.string().c_str());

	std::shared_ptr<diffs::core::kitchen> kitchen = diffs::core::kitchen::create();
	add_file_to_pantry(*kitchen, source_path);
	auto archive = load_diff(diff_path);
	archive->stock_kitchen(kitchen.get());

	diffs::core::item_definition item{size};
	std::vector<char> hash_data;
	if (!archive_diff::hashing::hexstring_to_data(hash, hash_data))
	{
		printf("Couldn't convert hash: %s into data.\n", hash.c_str());
		return 1;
	}
	std::string_view hash_view{hash_data.data(), hash_data.size()};
	auto hash_value =
		archive_diff::hashing::hash::import_hash_value(archive_diff::hashing::algorithm::sha256, hash_view);
	item = item.with_hash(hash_value);
	printf("Item: %s\n", item.to_string().c_str());

	kitchen->request_item(item);

	if (!kitchen->process_requested_items())
	{
		printf("Failed to process requested items\n");
		return 1;
	}

	kitchen->resume_slicing();

	auto prepared_item = kitchen->fetch_item(item);
	fs::create_directories(target_path);
	auto item_path                     = target_path / hash;
	std::shared_ptr<io::writer> writer = std::make_shared<io::file::binary_file_writer>(item_path.string());
	prepared_item->write(writer);

	prepared_item.reset();
	writer.reset();

	auto reader   = io::file::io_device::make_reader(item_path.string());
	auto new_item = diffs::core::create_definition_from_reader(reader);

	kitchen->cancel_slicing();

	if (!item.equals(new_item))
	{
		printf(
			"Extracted item at %s is incorrect.\n\tExpected: %s\n\tActual: %s\n",
			item_path.string().c_str(),
			item.to_string().c_str(),
			new_item.to_string().c_str());

		auto dependencies = get_dependencies(*archive, item);
		extract_items(*kitchen.get(), *archive, dependencies, target_path);
		printf("Done examining dependencies.\n");
		return 1;
	}
	printf("Ok.\n");

	return 0;
}
