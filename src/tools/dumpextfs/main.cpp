/**
 * @file main.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include "ext2fs/ext2_types.h"
#include "et/com_err.h"
#include "ext2fs/ext2_io.h"
#include <ext2fs/ext2_ext_attr.h>
#include "ext2fs/ext2fs.h"

#include <string>
#include <map>
#include <vector>
#include <iterator>

#include "load_ext4.h"
#include "file_details.h"
#include "dump_json.h"
#include "hash_utility.h"

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		printf("Usage: %s <ext4 path>\n", argv[0]);
		printf("OR     %s <ext4 path> <output json path>\n", argv[0]);
		exit(-1);
	}

	char *ext4_path   = argv[1];
	char *output_path = nullptr;
	FILE *stream      = stdout;
	if (argc == 3)
	{
		output_path = argv[2];
		stream      = fopen(output_path, "w");
	}

	std::vector<file_details> all_files;
	int retval = load_ext4(ext4_path, &all_files);
	if (retval)
	{
		printf("Failed to load data from ext4 file. retval: %d\n", retval);
		exit(retval);
	}

	dump_all_files(stream, all_files);

	if (output_path != nullptr)
	{
		fclose(stream);
	}

	return 0;
}
