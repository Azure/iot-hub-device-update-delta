#include <string>
#include <map>
#include <set>
#include <filesystem>

#include <stdio.h>

#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <archives/cpio_archives/cpio_archive.h>

void usage();

using namespace archive_diff;

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		usage();
		return -1;
	}

	try
	{
		std::string target_file(argv[1]);
		std::string formatStr(argv[2]);

		cpio_format format;
		if (formatStr.compare("new") == 0)
		{
			format = archive_diff::cpio_format::new_ascii;
		}
		else if (formatStr.compare("newc") == 0)
		{
			format = archive_diff::cpio_format::newc_ascii;
		}
		else
		{
			usage();
			return -1;
		}

		cpio_archive cpio(format);

		for (int i = 3; i < argc; i++)
		{
			auto file_path = std::string(argv[i]);

			if (!std::filesystem::exists(file_path))
			{
				printf("File %s does not exist.\n", file_path.c_str());
				return -1;
			}

			auto reader    = archive_diff::io::file::io_device::make_reader(file_path);
			uint32_t inode = cpio_file::get_inode(file_path) & 0xFFFFFFFF;

			cpio.add_file(file_path, inode, reader);
		}

		io::shared_writer file_writer = std::make_shared<archive_diff::io::file::binary_file_writer>(target_file);
		io::sequential::basic_writer_wrapper wrapper(file_writer);

		cpio.write(wrapper);

		return 0;
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

void usage() { printf("usage: <target cpio file> <new|newc> <file1> <file2> ... <fileN>\n"); }
