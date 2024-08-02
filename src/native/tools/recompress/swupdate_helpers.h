#pragma once

#include <string>
#include <vector>
#include <map>

#include <io/reader.h>

#include <language_support/include_filesystem.h>

#include "libconfig.h++"

struct image_entry_details
{
	std::string setting_path;
	std::string filename;
};

std::unique_ptr<libconfig::Config> load_config(archive_diff::io::reader &config_reader);

std::vector<image_entry_details> get_image_entries(libconfig::Config *config);

void write_new_sw_description(
	fs::path original_path,
	fs::path new_path,
	std::vector<image_entry_details> &images,
	std::map<std::string, std::string> &file_hash_map);
