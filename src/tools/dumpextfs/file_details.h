/**
 * @file file_details.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <vector>
#include "ext2fs/ext2_types.h"

struct file_region
{
	__u64 offset;
	__u64 length;
	bool all_zeroes;
	std::string hash_md5_string;
	std::string hash_sha256_string;
};

struct file_details
{
	std::string name;
	std::string full_path;
	__u64 size;
	std::string hash_md5_string;
	std::string hash_sha256_string;
	std::vector<file_region> regions;
};
