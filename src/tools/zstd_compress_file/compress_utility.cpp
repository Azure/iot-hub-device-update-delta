/**
 * @file compress_utility.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "compress_utility.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <filesystem>
#include <zstd.h>
#include <cstdio>

#include "binary_file_reader.h"
#include "binary_file_writer.h"
#include "wrapped_writer_sequential_writer.h"
#include "zstd_compression_writer.h"
#include "zstd_decompression_writer.h"
#include "wrapped_reader_sequential_reader.h"

#include "wrapped_writer_sequential_hashed_writer.h"

#include "get_file_hash.h"

void compress_file(fs::path uncompressed_path, std::vector<char> *dictionary, fs::path compressed_path)
{
	if (!fs::exists(uncompressed_path))
	{
		std::cout << uncompressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	io_utility::binary_file_writer file_writer(compressed_path.string());
	io_utility::wrapped_writer_sequential_writer wrapper(&file_writer);

	auto uncompressed_file_size = fs::file_size(uncompressed_path);

	io_utility::zstd_compression_writer writer(&wrapper, 1, 5, 3, uncompressed_file_size);

	if (dictionary)
	{
		printf("Setting dictionary.\n");
		writer.set_dictionary(dictionary);
	}

	io_utility::binary_file_reader reader(uncompressed_path.string());

	writer.write(&reader);
}

void compress_file(fs::path uncompressed_path, fs::path compressed_path)
{
	printf("Compressing %s to %s\n", uncompressed_path.string().c_str(), compressed_path.string().c_str());
	compress_file(uncompressed_path, nullptr, compressed_path);
}

void compress_file(fs::path uncompressed_path, fs::path delta_basis_path, fs::path compressed_path)
{
	if (!delta_basis_path.empty() && !fs::exists(delta_basis_path))
	{
		std::cout << delta_basis_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	printf(
		"Compressing %s to %s with basis %s\n",
		uncompressed_path.string().c_str(),
		compressed_path.string().c_str(),
		delta_basis_path.string().c_str());

	std::vector<char> delta_basis_data;
	if (!delta_basis_path.empty())
	{
		auto delta_basis_size = fs::file_size(delta_basis_path);
		delta_basis_data.reserve(delta_basis_size);
		std::ifstream delta_basis(delta_basis_path, std::ios::binary | std::ios::in);
		delta_basis.read(delta_basis_data.data(), delta_basis_size);
	}

	if (delta_basis_data.size() == 0)
	{
		compress_file(uncompressed_path, nullptr, compressed_path);
	}
	else
	{
		compress_file(uncompressed_path, &delta_basis_data, compressed_path);
	}
}

void verify_compression(fs::path uncompressed_path, fs::path *delta_basis_path, fs::path compressed_path)
{
	auto uncompressed_hash        = get_file_hash(uncompressed_path);
	auto uncompressed_hash_string = data_to_hexstring(uncompressed_hash.data(), uncompressed_hash.size());
	printf("Uncompressed file hash: %s\n", uncompressed_hash_string.c_str());

#ifdef WIN32
	std::vector<char> buffer;
	const size_t buffer_capacity = 1024;
	buffer.reserve(buffer_capacity);
	auto err = tmpnam_s(buffer.data(), buffer.capacity());
	if (err)
	{
		std::cout << "tmpnam_s() failed with " << err << std::endl;
		throw std::exception();
	}
	auto test_uncompressed_path = fs::path(buffer.data());
#else
	auto test_uncompressed_path = fs::path(std::tmpnam(nullptr));
#endif

	std::cout << "Testing decompression to temp path: " << test_uncompressed_path << std::endl;

	if (delta_basis_path)
	{
		decompress_file(compressed_path, *delta_basis_path, test_uncompressed_path);
	}
	else
	{
		decompress_file(compressed_path, test_uncompressed_path);
	}

	auto test_uncompressed_hash = get_file_hash(test_uncompressed_path);
	auto test_uncompressed_hash_string =
		data_to_hexstring(test_uncompressed_hash.data(), test_uncompressed_hash.size());
	printf("Test Uncompressed file hash: %s\n", test_uncompressed_hash_string.c_str());

	fs::remove(test_uncompressed_path);

	if (test_uncompressed_hash.size() != uncompressed_hash.size())
	{
		printf("Hashes of uncompressed data has mismatch size!\n");
		throw std::exception();
	}

	if (0 != memcmp(test_uncompressed_hash.data(), uncompressed_hash.data(), uncompressed_hash.size()))
	{
		printf("Hashes of uncompressed data has mismatch!\n");
		throw std::exception();
	}
}

void decompress_file(fs::path compressed_path, std::vector<char> *dictionary, fs::path uncompressed_path)
{
	if (!fs::exists(compressed_path))
	{
		std::cout << compressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	io_utility::binary_file_writer file_writer(uncompressed_path.string());
	io_utility::wrapped_writer_sequential_writer wrapper(&file_writer);
	auto compressed_file_size = fs::file_size(compressed_path);

	io_utility::zstd_decompression_writer writer(&wrapper, 1, 5, 3, compressed_file_size);

	if (dictionary)
	{
		printf("Setting dictionary.\n");
		writer.set_dictionary(dictionary);
	}

	io_utility::binary_file_reader reader(compressed_path.string());

	std::cout << "Decompressing " << compressed_path.string() << " to " << uncompressed_path.string() << std::endl;

	writer.write(&reader);
}

void decompress_file(fs::path compressed_path, fs::path uncompressed_path)
{
	if (!fs::exists(compressed_path))
	{
		std::cout << compressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	decompress_file(compressed_path, nullptr, uncompressed_path);
}

void decompress_file(fs::path compressed_path, fs::path delta_basis_path, fs::path uncompressed_path)
{
	if (!delta_basis_path.empty() && !fs::exists(delta_basis_path))
	{
		std::cout << delta_basis_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	std::vector<char> delta_basis_data;
	if (!delta_basis_path.empty())
	{
		auto delta_basis_size = fs::file_size(delta_basis_path);
		delta_basis_data.reserve(delta_basis_size);
		std::ifstream delta_basis(delta_basis_path, std::ios::binary | std::ios::in);
		delta_basis.read(delta_basis_data.data(), delta_basis_size);
	}

	decompress_file(compressed_path, &delta_basis_data, uncompressed_path);
}