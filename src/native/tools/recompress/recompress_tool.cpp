#include <string>
#include <map>
#include <set>
#include <fmt/core.h>

#include <stdio.h>

#include <hashing/hasher.h>

#include <language_support/include_filesystem.h>
#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <io/file/temp_file.h>

#include <archives/cpio_archives/cpio_archive.h>
#include <archives/cpio_archives/cpio_file.h>

#include "recompress.h"
#include "swupdate_helpers.h"

const char *SW_DESCRPTION_FILE_NAME = "sw-description";

void usage();
void folder_cmd(fs::path &source, fs::path &dest);
void swu_cmd(fs::path &source, fs::path &dest, std::string *signing_cmd);

int main(int argc, char **argv)
{
	if (argc != 4 && argc != 5)
	{
		usage();
		return -1;
	}

	try
	{
		std::string command = argv[1];

		if (command.compare("folder") == 0)
		{
			fs::path source = argv[2];
			fs::path dest   = argv[3];
			folder_cmd(source, dest);
			return 0;
		}

		if (command.compare("swu") == 0)
		{
			fs::path source = argv[2];
			fs::path dest   = argv[3];

			if (argc == 5)
			{
				std::string signing_cmd = argv[4];

				swu_cmd(source, dest, &signing_cmd);
			}
			else
			{
				swu_cmd(source, dest, nullptr);
			}

			printf("Finished successfully.\n");
			return 0;
		}

		usage();

		return -1;
	}
	catch (std::exception &e)
	{
		if (e.what() == nullptr)
		{
			printf("Failed. Caught an exception.\n");
		}
		else
		{
			printf("Failed. Caught an exception. Msg: %s\n", e.what());
		}

		return 1;
	}
}

void usage() { printf("usage: recompress <folder|swu> <source> <destination> [<signing command>]\n"); }

void folder_cmd(fs::path &source, fs::path &dest)
{
	auto sw_description_source = source / SW_DESCRPTION_FILE_NAME;
	auto sw_description_reader = archive_diff::io::file::io_device::make_reader(sw_description_source.string());

	auto config = load_config(sw_description_reader);

	auto image_entries = get_image_entries(config.get());

	std::set<std::string> image_files;
	std::map<std::string, std::string> file_hashes;

	for (const auto &entry : image_entries)
	{
		printf("setting_path: %s, filename: %s\n", entry.setting_path.c_str(), entry.filename.c_str());

		image_files.insert(entry.filename);
	}

	if (!fs::is_directory(dest))
	{
		fs::create_directories(dest);
	}

	for (auto &filename : image_files)
	{
		auto source_path = source / filename;
		auto dest_path   = dest / filename;

		recompress(source_path, dest_path);
		auto dest2 = recompress_using_reader(source_path, dest_path);

		auto hash1 = get_filehash_string(dest_path);
		auto hash2 = get_filehash_string(dest2);

		if (!compare_file_hashes(dest_path, dest2, hash1, hash2))
		{
			return;
		}

		file_hashes[filename] = hash1;
	}

	auto sw_description_target = dest / SW_DESCRPTION_FILE_NAME;

	write_new_sw_description(sw_description_source, sw_description_target, image_entries, file_hashes);
}

std::string replace(const std::string &s, char find_value, char replace_value)
{
	std::string new_string = s;

	while (true)
	{
		auto pos = new_string.find(find_value);
		if (pos == std::string::npos)
		{
			break;
		}

		new_string[pos] = replace_value;
	}

	return new_string;
}

std::string replace(const std::string &s, std::span<char> find_values, char replace_value)
{
	const std::string *prev_string = &s;
	std::string new_string;

	for (auto i : find_values)
	{
		new_string  = replace(*prev_string, i, replace_value);
		prev_string = &new_string;
	}

	return new_string;
}

archive_diff::hashing::hash hash_reader(archive_diff::io::reader &reader)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);

	const size_t block_size = 8 * 1024;

	char data[block_size];

	auto remaining = reader.size();
	uint64_t offset{0};

	while (remaining)
	{
		auto to_read = static_cast<size_t>(std::min<uint64_t>(remaining, sizeof(data)));
		reader.read(offset, std::span{data, to_read});
		hasher.hash_data(std::string_view{data, to_read});

		remaining -= to_read;
		offset += to_read;
	}

	return hasher.get_hash();
}

void swu_cmd(fs::path &source, fs::path &dest, std::string *signing_cmd)
{
	{
		archive_diff::cpio_archive archive;
		if (!fs::is_regular_file(source))
		{
			printf("No such file exists: %s\n", source.string().c_str());
			return;
		}

		auto reader = archive_diff::io::file::io_device::make_reader(source.string());
		if (!archive.try_read(reader))
		{
			return;
		}

		printf("Writing new swu to %s\n", dest.string().c_str());

		auto sw_description_reader = archive.get_payload_reader(SW_DESCRPTION_FILE_NAME);

		auto config        = load_config(sw_description_reader);
		auto &root         = config->getRoot();
		auto image_entries = get_image_entries(config.get());

		std::map<std::string, std::string> file_to_hash_map;

		for (auto &entry : image_entries)
		{
			auto file = entry.filename;

			if (!file_to_hash_map.contains(file))
			{
				auto payload_reader = archive.get_payload_reader(file);

				auto temp_file = std::make_shared<archive_diff::io::file::temp_file>();

				auto writer = archive_diff::io::file::temp_file_writer::make_shared(temp_file);

				recompress(payload_reader, writer);

				auto recompressed_reader = archive_diff::io::file::temp_file_io_device::make_reader(temp_file);

				archive.set_payload_reader(file, recompressed_reader);

				auto hash = hash_reader(recompressed_reader);

				file_to_hash_map.insert(std::make_pair(file, hash.get_string()));
			}

			auto &entry_setting = root.lookup(entry.setting_path);
			if (!entry_setting.exists("sha256"))
			{
				entry_setting.add("sha256", libconfig::Setting::TypeString);
			}

			auto hash_string        = file_to_hash_map[file];
			entry_setting["sha256"] = hash_string;
		}

		fs::create_directories(dest.parent_path());

		std::shared_ptr<archive_diff::io::writer> writer =
			std::make_shared<archive_diff::io::file::binary_file_writer>(dest.string());
		archive_diff::io::sequential::basic_writer_wrapper wrapper(writer);

		// Update the sw-descripton file
		char delimiters[]   = {'/', '\\', ':'};
		auto source_escaped = replace(source.string(), delimiters, '_');
		auto description_path =
			fs::temp_directory_path() / "recompress_tool" / source_escaped / SW_DESCRPTION_FILE_NAME;

		printf("Writing new sw-description to: %s\n", description_path.string().c_str());
		fs::create_directories(description_path.parent_path());
		config->writeFile(description_path.string().c_str());

		auto description_reader = archive_diff::io::file::io_device::make_reader(description_path.string());
		archive.set_payload_reader("sw-description", description_reader);

		const char *SIG_FILE_NAME = "sw-description.sig";

		if (signing_cmd == nullptr)
		{
			if (archive.has_file(SIG_FILE_NAME))
			{
				printf("No signing mechanism passed in, deleting sw-description.sig\n");
				archive.remove_file("sw-description.sig");
			}
		}
		else
		{
			fs::path sig_path = description_path.string() + ".sig";

			std::string signing_cmd_with_args =
				fmt::format("{} {} {}", *signing_cmd, description_path.string().c_str(), sig_path.string());

			::system(signing_cmd_with_args.c_str());

			auto sig_reader = archive_diff::io::file::io_device::make_reader(sig_path.string());

			if (archive.has_file(SIG_FILE_NAME))
			{
				archive.set_payload_reader(SIG_FILE_NAME, sig_reader);
			}
			else
			{
				auto index            = archive.get_file_index(SW_DESCRPTION_FILE_NAME);
				auto sig_inode        = archive_diff::cpio_file::get_inode(sig_path);
				uint32_t sig_inode_32 = sig_inode & 0xFFFFFFFF;
				archive.insert_file_at(index, SIG_FILE_NAME, sig_inode_32, sig_reader);
			}
		}

		archive.write(wrapper);
	}
}