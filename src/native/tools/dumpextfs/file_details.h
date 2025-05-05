/**
 * @file file_details.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include <vector>
#include <optional>
#include "ext2fs/ext2_types.h"

struct file_region
{
	std::optional<uint64_t> offset;
	uint64_t length{};
	bool all_zeroes;
	std::string hash_sha256_string;
};

struct file_details
{
	std::string name;
	std::string full_path;
	uint64_t length{};
	std::string hash_sha256_string;
	std::vector<file_region> regions;
};

struct archive_details
{
	uint64_t length{};
	std::string hash_sha256_string;

	std::vector<file_details> files;
};