#include "read_test_file.h"

#include <io/file/io_device.h>

std::vector<char> read_test_file(fs::path path)
{
	auto reader = archive_diff::io::file::io_device::make_reader(path.string());
	std::vector<char> data;
	reader.read_all(data);
	return data;
}