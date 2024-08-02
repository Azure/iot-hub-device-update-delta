/**
 * @file dump_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <io/file/binary_file_writer.h>

#include <language_support/include_filesystem.h>

namespace archive_diff::test_utility
{
	
	void write_to_file(std::vector<char>& data, fs::path destination)
	{
		printf("Writing %s\n", destination.string().c_str());
		io::file::binary_file_writer writer(destination.string());

		writer.write(0, std::string_view{data.data(), data.size()});
	}
}