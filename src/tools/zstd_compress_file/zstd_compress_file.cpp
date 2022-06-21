/**
 * @file zstd_compress_file.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

#include "compress_utility.h"

void usage(char *executable_name);

enum class operation
{
	invalid = -1,
	compress,
	decompress,
	compress_with_basis,
	decompress_with_basis,
};

int parse_command_line(
	int argc,
	char **argv,
	operation *op,
	fs::path *uncompressed_path,
	fs::path *compressed_path,
	fs::path *delta_basis_path);

int main(int argc, char **argv)
{
	fs::path uncompressed_path;
	fs::path delta_basis_path;
	fs::path compressed_path;

	operation op{operation::invalid};
	int ret = parse_command_line(argc, argv, &op, &uncompressed_path, &compressed_path, &delta_basis_path);

	if (ret != 0)
	{
		usage(argv[0]);
		return ret;
	}

	try
	{
		switch (op)
		{
		case operation::invalid:
			usage(argv[0]);
			return -1;
		case operation::compress:
			compress_file(uncompressed_path, compressed_path);
			verify_compression(uncompressed_path, nullptr, compressed_path);
			break;
		case operation::decompress:
			decompress_file(compressed_path, uncompressed_path);
			break;
		case operation::compress_with_basis:
			compress_file(uncompressed_path, delta_basis_path, compressed_path);
			verify_compression(uncompressed_path, &delta_basis_path, compressed_path);
			break;
		case operation::decompress_with_basis:
			decompress_file(compressed_path, delta_basis_path, uncompressed_path);
			break;
		}
	}
	catch (std::exception &)
	{
		printf("Failed.\n");
		return 2;
	}

	printf("Success.\n");

	return 0;
}

#define DECOMPRESS_SWITCH "-d"

int parse_command_line(
	int argc,
	char **argv,
	operation *op,
	fs::path *uncompressed_path,
	fs::path *compressed_path,
	fs::path *delta_basis_path)
{
	switch (argc)
	{
	case 3: // <uncompressed> <compressed>
		*op                = operation::compress;
		*uncompressed_path = argv[1];
		*compressed_path   = argv[2];
		break;
	case 4:
		if (0 == strcmp(argv[1], DECOMPRESS_SWITCH)) // -d <compressed> <uncompressed>
		{
			*op                = operation::decompress;
			*compressed_path   = argv[2];
			*uncompressed_path = argv[3];
		}
		else
		{
			*op                = operation::compress_with_basis;
			*uncompressed_path = argv[1];
			*delta_basis_path  = argv[2];
			*compressed_path   = argv[3];
		}
		break;
	case 5:
		if (0 == strcmp(argv[1], DECOMPRESS_SWITCH))
		{
			*op                = operation::decompress_with_basis;
			*compressed_path   = argv[2];
			*delta_basis_path  = argv[3];
			*uncompressed_path = argv[4];
		}
		else
		{
			*op = operation::invalid;
			return -1;
		}
	}

	return 0;
}

void usage(char *executable_name)
{
	std::cout << "Usage: " << executable_name << " <uncompressed> <compressed>" << std::endl
			  << "    or " << executable_name << " <uncompressed> <basis> <compressed>" << std::endl
			  << "    or " << executable_name << " -d <compressed> <uncompressed>" << std::endl
			  << "    or " << executable_name << " -d <compressed> <basis> <uncompressed>" << std::endl;
}
