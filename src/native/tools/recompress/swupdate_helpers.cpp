#include "swupdate_helpers.h"

std::unique_ptr<libconfig::Config> load_config(archive_diff::io::reader &config_reader)
{
	auto config = std::make_unique<libconfig::Config>();

	std::vector<char> config_data;
	config_data.resize(config_reader.size() + 1);

	config_reader.read_all(config_data);

	config->readString(config_data.data());

	return config;
}

std::vector<image_entry_details> get_image_entries(libconfig::Config *config)
{
	std::vector<image_entry_details> entries;

	auto &root           = config->getRoot();
	auto &software_group = *root.begin();

	std::string software_name = software_group.getName();
	if (software_name.compare("software") != 0)
	{
		printf("Top level group was not 'software'. Found: %s\n", software_name.c_str());
		throw std::exception();
	}

	printf("Found 'software' group.\n");

	for (const auto &software_child_group : software_group)
	{
		if (software_child_group.getType() != libconfig::Setting::TypeGroup)
		{
			continue;
		}

		if (!software_child_group.exists("stable"))
		{
			continue;
		}

		const auto &stable_group = software_child_group.lookup("stable");

		std::string settingName = software_child_group.getName();
		printf("Found 'stable' group in %s group.\n", settingName.c_str());

		if (stable_group.getType() != libconfig::Setting::TypeGroup)
		{
			printf("stable getting is not a group.\n");
			throw std::exception();
		}

		for (const auto &stable_child_group : stable_group)
		{
			if (!stable_child_group.exists("images"))
			{
				break;
			}

			auto &images_list = stable_child_group.lookup("images");
			if (images_list.getType() != libconfig::Setting::TypeList)
			{
				printf("Found an images setting that wasn't a list on line: %u\n", images_list.getSourceLine());
				throw std::exception();
			}

			std::string stable_child_group_name = stable_child_group.getName();
			printf("Found an images list in %s.\n", stable_child_group_name.c_str());

			for (const auto &image_entry : images_list)
			{
				bool compressed{false};
				std::string value;
				if (!image_entry.lookupValue("compressed", value))
				{
					printf("Found an entry with no compressed entry... skipping.\n");
				}

				if (value.compare("zlib") == 0 || value.compare("gz") == 0 || value.compare("true") == 0)
				{
					compressed = true;
				}

				if (!compressed)
				{
					printf("Found an entry that wasn't compressed... skipping.\n");
				}

				std::string filename_value;
				if (!image_entry.lookupValue("filename", filename_value))
				{
					printf("Couldn't find value for filename on entry on line: %u\n", images_list.getSourceLine());
					throw std::exception();
				}

				auto path      = images_list.getPath();
				auto entryPath = image_entry.getPath();
				entries.push_back(image_entry_details{entryPath, filename_value});
			}
		}
	}

	return entries;
}

void write_new_sw_description(
	fs::path original_path,
	fs::path new_path,
	std::vector<image_entry_details> &images,
	std::map<std::string, std::string> &file_hash_map)
{
	libconfig::Config config;

	config.readFile(original_path.string());

	for (const auto &entry : images)
	{
		auto &setting     = config.lookup(entry.setting_path);
		setting["sha256"] = file_hash_map[entry.filename];
	}

	config.writeFile(new_path.string());

	printf("Writing updated sw-description to %s\n", new_path.string().c_str());
}