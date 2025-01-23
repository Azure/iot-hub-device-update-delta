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

int dump_pantry(Json::Value &pantry, std::ostream &ostream, Json::StreamWriter *writer)
{
	ostream << " \"Pantry\": ";
	ostream << " [\n";
	for (int i_item = 0; i_item < (int)pantry.size(); i_item++)
	{
		auto item = pantry[i_item];

		ostream << "  ";
		writer->write(item, &ostream);

		if (i_item != (int)(pantry.size() - 1))
		{
			ostream << ",\n";
		}
		else
		{
			ostream << "\n";
		}
	}

	ostream << " ]";

	return 0;
}

int dump_supported_recipes(Json::Value &supported_recipe_types, std::ostream &ostream, Json::StreamWriter *writer)
{
	ostream << " \"SupportedRecipeTypes\":\n [\n";
	for (int i_recipe_type = 0; i_recipe_type < (int)supported_recipe_types.size(); i_recipe_type++)
	{
		auto recipe_type = supported_recipe_types[i_recipe_type];

		ostream << "  ";
		writer->write(recipe_type, &ostream);

		if (i_recipe_type != (int)(supported_recipe_types.size() - 1))
		{
			ostream << ",\n";
		}
		else
		{
			ostream << "\n";
		}
	}

	ostream << " ]";

	return 0;
}

int dump_recipe(Json::Value& recipe, std::ostream& ostream, Json::StreamWriter* writer)
{
	auto members = recipe.getMemberNames();
	std::set<std::string> member_set(members.begin(), members.end());

	bool first = true;

	ostream << "{";

	if (member_set.contains("Name"))
	{
		first = false;

		ostream << "\"Name\":";
		writer->write(recipe["Name"], &ostream);
	}

	if (member_set.contains("Result"))
	{
		if (!first)
		{
			ostream << ",";
		}
		first = false;

		ostream << "\"Result\":";
		writer->write(recipe["Result"], &ostream);
	}

	if (member_set.contains("NumberIngredients"))
	{
		if (!first)
		{
			ostream << ",";
		}
		first = false;

		ostream << "\"NumberIngredients\":";
		writer->write(recipe["NumberIngredients"], &ostream);
	}

	if (member_set.contains("ItemIngredients"))
	{
		if (!first)
		{
			ostream << ",";
		}
		first = false;

		ostream << "\"ItemIngredients\":";
		writer->write(recipe["ItemIngredients"], &ostream);
	}

	ostream << "}";

	return 0;
}

int dump_cookbook(Json::Value &cookbook, std::ostream &ostream, Json::StreamWriter *writer)
{
	ostream << " \"Recipes\":\n [\n";
	for (int i_recipe = 0; i_recipe < (int)cookbook.size(); i_recipe++)
	{
		auto recipe = cookbook[i_recipe];

		ostream << "  ";
		int ret = dump_recipe(recipe, ostream, writer);
		if (ret != 0)
		{
			return ret;
		}

		if (i_recipe != (int)(cookbook.size() - 1))
		{
			ostream << ",\n";
		}
		else
		{
			ostream << "\n";
		}
	}

	ostream << " ]";

	return 0;
}


int dump_diff(archive_diff::diffs::core::archive *to_dump, std::ostream &ostream)
{
	auto root = to_dump->to_json();

	Json::StreamWriterBuilder builder;
	builder["indentation"] = "";
	const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    ostream << "{\n";

	auto target = root["TargetItem"];

	ostream << " \"TargetItem\": ";
	writer->write(target, &ostream);

	auto members = root.getMemberNames();

	std::set<std::string> member_set(members.begin(), members.end());

	if (member_set.contains("SourceItem"))
	{
		ostream << ",\n";
		auto source = root["SourceItem"];
		ostream << " \"SourceItem\": ";
		writer->write(source, &ostream);
	}

	if (member_set.contains("Pantry"))
	{
		ostream << ",\n";
		dump_pantry(root["Pantry"], ostream, writer.get());
	}

	if (!g_skip_recipes)
	{
		if (member_set.contains("SupportedRecipeTypes"))
		{
			ostream << ",\n";
			dump_supported_recipes(root["SupportedRecipeTypes"], ostream, writer.get());
		}

		if (member_set.contains("Cookbook"))
		{
			ostream << ",\n";
			dump_cookbook(root["Cookbook"], ostream, writer.get());
		}
	}

	ostream << "\n";

	ostream << "}\n";

	return 0;
}