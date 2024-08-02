/**
 * @file dumpdiff.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string>
#include <iostream>

#include <errors/user_exception.h>
#include <io/file/io_device.h>

#include <language_support/include_filesystem.h>

#include <diffs/serialization/legacy/deserializer.h>
#include <diffs/serialization/standard/deserializer.h>

int dump_diff(archive_diff::diffs::core::archive *to_dump, std::ostream &ostream);

void usage()
{
	std::cout << "Usage: dumpdiff <diff path>" << std::endl;
	std::cout << " or dumpdiff <diff path> --skip_recipes" << std::endl;
}

bool g_skip_recipes{false};

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		usage();
		return 1;
	}

	if (argc > 2)
	{
		if (0 != strcmp(argv[2], "--skip_recipes"))
		{
			usage();
			return 1;
		}

		g_skip_recipes = true;
	}

	auto path = fs::path(argv[1]);

	if (!fs::exists(path))
	{
		printf("No file found at path: %s\n", path.string().c_str());
		return 1;
	}

	auto reader = archive_diff::io::file::io_device::make_reader(path.string());

	std::shared_ptr<archive_diff::diffs::core::archive> archive;

	std::string reason;
	if (archive_diff::diffs::serialization::standard::deserializer::is_this_format(reader, &reason))
	{
		archive_diff::diffs::serialization::standard::deserializer deserializer;
		try
		{
			deserializer.read(reader);
			archive = deserializer.get_archive();
		}
		catch (archive_diff::errors::user_exception &e)
		{
			std::cout << "Expection while loading diff. Code: " << std::to_string(static_cast<uint16_t>(e.get_error()));
			std::cout << ", Message: " << e.get_message() << std::endl;
			return 1;
		}
	}
	else
	{
		archive_diff::diffs::serialization::legacy::deserializer deserializer;
		try
		{
			deserializer.read(reader);
			archive = deserializer.get_archive();
		}
		catch (archive_diff::errors::user_exception &e)
		{
			std::cout << "Expection while loading diff. Code: " << std::to_string(static_cast<uint16_t>(e.get_error()));
			std::cout << ", Message: " << e.get_message() << std::endl;
			return 1;
		}
	}

	return dump_diff(archive.get(), std::cout);
}

int dump_diff(archive_diff::diffs::core::archive *to_dump, std::ostream &ostream)
{
	archive_diff::diffs::serialization::legacy::deserializer deserializer;

	auto target_item = to_dump->get_archive_item();
	ostream << "Target: " << target_item.to_string() << std::endl;

	auto source_item = to_dump->get_source_item();
	if (source_item.size())
	{
		ostream << "Source: " << source_item.to_string() << std::endl;
	}

	auto &pantry = to_dump->get_pantry();

	auto pantry_items = pantry->get_items();

	if (!pantry_items.empty())
	{
		ostream << "Pantry:" << std::endl;
		for (const auto &pantry_entry : pantry_items)
		{
			ostream << "\t" << pantry_entry.second->to_string() << std::endl;
		}
		ostream << std::endl;
	}

	auto &cookbook = to_dump->get_cookbook();

	if (!g_skip_recipes)
	{
		ostream << "Recipe Types: " << std::endl;
		auto supported_recipe_types = to_dump->get_supported_recipe_type_count();
		for (uint32_t i = 0; i < supported_recipe_types; i++)
		{
			ostream << "\t" << std::to_string(i) << ") " << to_dump->get_supported_type_name(i) << std::endl;
		}

		ostream << "Cookbook:" << std::endl;
		auto recipe_set_map = cookbook->get_all_recipes();
		if (recipe_set_map.empty())
		{
			ostream << "\tNo recipes in cookbook." << std::endl;
		}

		for (auto &entry : recipe_set_map)
		{
			auto &recipes = entry.second;
			for (const auto &recipe : recipes)
			{
				ostream << "\t" << recipe->to_string() << std::endl;
			}
		}
	}

	return 0;
}