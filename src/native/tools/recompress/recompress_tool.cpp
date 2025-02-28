#include <string>
#include <map>
#include <set>
#include <filesystem>
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
bool swu_cmd(fs::path &source, fs::path &dest, std::string *signing_cmd);
bool generate_description_sig(fs::path &file_path, std::string &signing_cmd, archive_diff::cpio_archive &archive);

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

				if (!swu_cmd(source, dest, &signing_cmd))
				{
					printf("Failed to recompress.");
					return -1;
				}
			}
			else
			{
				if (!swu_cmd(source, dest, nullptr))
				{
					printf("Failed to recompress.");
					return -1;
				}
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

const char *SIG_FILE_NAME = "sw-description.sig";

bool swu_cmd(fs::path &source, fs::path &dest, std::string *signing_cmd)
{
	{
		archive_diff::cpio_archive archive;
		if (!fs::is_regular_file(source))
		{
			printf("No such file exists: %s\n", source.string().c_str());
			return false;
		}

		auto reader = archive_diff::io::file::io_device::make_reader(source.string());
		if (!archive.try_read(reader))
		{
			return false;
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

				file_to_hash_map.insert(std::make_pair(file, hash.get_data_string()));
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
			if (!generate_description_sig(description_path, *signing_cmd, archive))
			{
				return false;
			}
		}

		archive.write(wrapper);
	}

	return true;
}

#ifdef WIN32
	#define POPEN  _popen
	#define PCLOSE _pclose
	#define POPEN_READ_MODE "rt"
#else
	#define POPEN  popen
	#define PCLOSE pclose
	#define POPEN_READ_MODE "r"
#endif

#ifdef WIN32
static bool contains_space(const std::string &str) { return str.find(' ') != std::string::npos; }

static bool ends_with(const std::string &str, const std::string &ending)
{
	if (ending.length() > str.length())
	{
		return false;
	}

	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
}
#endif

bool sign_file(const std::string &cmd, const std::string &file_path, const std::string &sig_path)
{
	char buffer[128];

	std::string cmd_with_args = fmt::format("{} {} {}", cmd, file_path, sig_path);

	printf("Signing with command: %s\n", cmd.c_str());
	printf("File: %s\n", file_path.c_str());
	printf("Sig File: %s\n", sig_path.c_str());

	FILE *pipe;
	if ((pipe = POPEN(cmd_with_args.c_str(), POPEN_READ_MODE)) == nullptr)
	{
		printf("Failed to open pipe.\n");
		return false;
	}

	while (fgets(buffer, sizeof(buffer), pipe))
	{
		puts(buffer);
	}

	int end_of_file = feof(pipe);
	int ret_val     = PCLOSE(pipe);

	if (ret_val != 0)
	{
		printf("Failed to close pipe. pclose(): %d\n", ret_val);
		return false;
	}

	if (end_of_file == 0)
	{
		printf("Failed to close pipe. feof(): 0\n");
		return false;
	}

	return true;
}

#ifdef WIN32

enum class WslPathType
{
	Windows = 0,
	Linux   = 1,
};

bool get_wsl_path(const std::string &path, std::string *wsl_path, WslPathType wslPathType)
{
	char buffer[128];
	std::string new_wsl_path;

	std::filesystem::path absolute = std::filesystem::absolute(path);
	std::string absoulte_str       = absolute.string();
	const std::string *pathValue   = &absoulte_str;
	char wslPathTypeSwitch         = 'w';

	if (wslPathType == WslPathType::Linux)
	{
		wslPathTypeSwitch = 'a';
	}
	else
	{
		pathValue = &path;
	}

	std::string cmd = "wsl wslpath -" + std::string(1, wslPathTypeSwitch) + " \"" + *pathValue + "\"";

	printf("Determining wsl path by calling: %s\n", cmd.c_str());
	FILE *pipe;
	if ((pipe = POPEN(cmd.c_str(), "rt")) == nullptr)
	{
		return false;
	}

	while (fgets(buffer, sizeof(buffer), pipe))
	{
		puts(buffer);
		new_wsl_path += std::string(buffer);
	}

	int end_of_file = feof(pipe);
	int ret_val     = PCLOSE(pipe);

	if (ret_val != 0)
	{
		return false;
	}

	*wsl_path = new_wsl_path.substr(0, new_wsl_path.length() - 1);
	printf("Determined wsl path: %s\n", wsl_path->c_str());

	return end_of_file != 0;
}

bool get_linux_path_of_file(const std::string &path, std::string *linux_path)
{
	std::string wsl_path;

	// Is this an existing script in the WSL?
	if (!get_wsl_path(path, &wsl_path, WslPathType::Windows))
	{
		return false;
	}

	printf("path: %s\n", wsl_path.c_str());

	if (fs::exists(wsl_path))
	{
		printf("Found script in WSL: %s\n", wsl_path.c_str());
		*linux_path = path;
		return true;
	}

	if (!get_wsl_path(path, &wsl_path, WslPathType::Linux))
	{
		return false;
	}

	*linux_path = wsl_path;
	return true;
}

bool sign_file_using_wsl(const std::string &cmd, const std::string &file_path, const std::string &sig_path)
{
	std::string wsl_cmd_path;

	printf("cmd: %s\n", cmd.c_str());

	if (!get_linux_path_of_file(cmd, &wsl_cmd_path))
	{
		return false;
	}

	std::string wsl_file_path;
	if (!get_wsl_path(file_path, &wsl_file_path, WslPathType::Linux))
	{
		return false;
	}

	std::string wsl_sig_path;
	if (!get_wsl_path(sig_path, &wsl_sig_path, WslPathType::Linux))
	{
		return false;
	}

	std::string wsl_cmd = "wsl " + wsl_cmd_path;

	return sign_file(wsl_cmd, wsl_file_path, wsl_sig_path);
}
#endif

bool generate_description_sig(fs::path &file_path, std::string &signing_cmd, archive_diff::cpio_archive &archive)
{
	fs::path sig_path = file_path.string() + ".sig";

	std::string output;

#ifdef WIN32
	if (!contains_space(signing_cmd) && ends_with(signing_cmd, ".sh"))
	{
		printf("Signing using WSL.\n");
		if (!sign_file_using_wsl(signing_cmd, file_path.string(), sig_path.string()))
		{
			printf("Failed to sign file.\n");
			return false;
		}
	}
	else
#endif
	{
		printf("Signing.\n");
		if (!sign_file(signing_cmd, file_path.string(), sig_path.string()))
		{
			return false;
		}
	}

	auto sig_reader = archive_diff::io::file::io_device::make_reader(sig_path.string());

	printf("Setting %s as %s in SWU.\n", sig_path.string().c_str(), SIG_FILE_NAME);
	if (archive.has_file(SIG_FILE_NAME))
	{
		printf("Overriding existing...");
		archive.set_payload_reader(SIG_FILE_NAME, sig_reader);
		printf("done.\n");
	}
	else
	{
		printf("Adding new entry...");
		auto index            = archive.get_file_index(SW_DESCRPTION_FILE_NAME);
		auto sig_inode        = archive_diff::cpio_file::get_inode(sig_path);
		uint32_t sig_inode_32 = sig_inode & 0xFFFFFFFF;
		archive.insert_file_at(index, SIG_FILE_NAME, sig_inode_32, sig_reader);
		printf("done.\n");
	}

	return true;
}