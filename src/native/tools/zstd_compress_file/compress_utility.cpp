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

#include <language_support/include_filesystem.h>

#include <zstd.h>
#include <cstdio>

#include <io/compressed/zstd_compression_writer.h>
#include <io/compressed/zstd_decompression_writer.h>
#include <io/file/io_device.h>
#include <io/file/binary_file_writer.h>
#include <io/hashed/hashed_sequential_writer.h>
#include <io/sequential/basic_reader_wrapper.h>
#include <io/sequential/basic_writer_wrapper.h>

#include <hashing/hexstring_convert.h>

#include "get_file_hash.h"

#ifndef WIN32
	#include <unistd.h>
#endif

void compress_file(
	fs::path uncompressed_path, std::shared_ptr<std::vector<char>> &dictionary_data, fs::path compressed_path)
{
	if (!fs::exists(uncompressed_path))
	{
		std::cout << uncompressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	std::shared_ptr<archive_diff::io::writer> file_writer =
		std::make_shared<archive_diff::io::file::binary_file_writer>(compressed_path.string());
	std::shared_ptr<archive_diff::io::sequential::writer> sequential_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(file_writer);

	auto uncompressed_file_size = fs::file_size(uncompressed_path);

	std::unique_ptr<archive_diff::io::sequential::writer> writer;

	if (dictionary_data.get())
	{
		printf("Setting dictionary.\n");
		archive_diff::io::compressed::compression_dictionary dictionary{dictionary_data};

		writer = std::make_unique<archive_diff::io::compressed::zstd_compression_writer>(
			sequential_writer, 3, uncompressed_file_size, std::move(dictionary));
	}
	else
	{
		writer = std::make_unique<archive_diff::io::compressed::zstd_compression_writer>(
			sequential_writer, 3, uncompressed_file_size);
	}

	auto reader = archive_diff::io::file::io_device::make_reader(uncompressed_path.string());

	writer->write(reader);
}

void compress_file(fs::path uncompressed_path, fs::path compressed_path)
{
	printf("Compressing %s to %s\n", uncompressed_path.string().c_str(), compressed_path.string().c_str());
	compress_file(uncompressed_path, L"", compressed_path);
}

void compress_file(fs::path uncompressed_path, fs::path delta_basis_path, fs::path compressed_path)
{
	if (!delta_basis_path.empty() && !fs::exists(delta_basis_path))
	{
		std::cout << delta_basis_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	printf("Compressing %s to %s.\n", uncompressed_path.string().c_str(), compressed_path.string().c_str());

	if (!delta_basis_path.empty())
	{
		printf("Using basis: %s\n", delta_basis_path.string().c_str());
	}

	std::shared_ptr<std::vector<char>> delta_basis_data;
	if (!delta_basis_path.empty())
	{
		delta_basis_data      = std::make_shared<std::vector<char>>();
		auto delta_basis_size = fs::file_size(delta_basis_path);
		delta_basis_data->resize(delta_basis_size);
		std::ifstream delta_basis(delta_basis_path, std::ios::binary | std::ios::in);
		delta_basis.read(delta_basis_data->data(), delta_basis_size);
	}

	compress_file(uncompressed_path, delta_basis_data, compressed_path);
}

void verify_compression(fs::path uncompressed_path, fs::path *delta_basis_path, fs::path compressed_path)
{
	auto uncompressed_hash = get_file_hash(uncompressed_path);
	auto uncompressed_hash_string =
		archive_diff::hashing::data_to_hexstring(uncompressed_hash.data(), uncompressed_hash.size());
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
	char name_template[] = "compress_verify_XXXXXX";
	int fd               = mkstemp(name_template);
	if (fd == -1)
	{
		std::cout << "Couldn't open temp file to verify compression. Errno: " << std::to_string(errno) << std::endl;
		throw std::exception();
	}
	close(fd);
	auto test_uncompressed_path = fs::temp_directory_path() / name_template;
#endif

	std::cout << "Testing decompression to temp path: " << test_uncompressed_path << std::endl;

	if (delta_basis_path)
	{
		decompress_file(compressed_path, *delta_basis_path, test_uncompressed_path);
	}
	else
	{
		decompress_file(compressed_path, L"", test_uncompressed_path);
	}

	auto test_uncompressed_hash = get_file_hash(test_uncompressed_path);
	auto test_uncompressed_hash_string =
		archive_diff::hashing::data_to_hexstring(test_uncompressed_hash.data(), test_uncompressed_hash.size());
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

void decompress_file(
	fs::path compressed_path, std::shared_ptr<std::vector<char>> &dictionary_data, fs::path uncompressed_path)
{
	if (!fs::exists(compressed_path))
	{
		std::cout << compressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	std::shared_ptr<archive_diff::io::writer> file_writer =
		std::make_shared<archive_diff::io::file::binary_file_writer>(uncompressed_path.string());
	std::shared_ptr<archive_diff::io::sequential::writer> sequential_writer =
		std::make_shared<archive_diff::io::sequential::basic_writer_wrapper>(file_writer);

	std::unique_ptr<archive_diff::io::compressed::zstd_decompression_writer> writer;

	if (dictionary_data.get())
	{
		printf("Setting dictionary.\n");
		archive_diff::io::compressed::compression_dictionary dictionary{dictionary_data};

		writer = std::make_unique<archive_diff::io::compressed::zstd_decompression_writer>(
			sequential_writer, std::move(dictionary));
	}
	else
	{
		writer = std::make_unique<archive_diff::io::compressed::zstd_decompression_writer>(sequential_writer);
	}

	auto reader = archive_diff::io::file::io_device::make_reader(compressed_path.string());

	std::cout << "Decompressing " << compressed_path.string() << " to " << uncompressed_path.string() << std::endl;

	writer->write(reader);
}

void decompress_file(fs::path compressed_path, fs::path uncompressed_path)
{
	if (!fs::exists(compressed_path))
	{
		std::cout << compressed_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	decompress_file(compressed_path, L"", uncompressed_path);
}

void decompress_file(fs::path compressed_path, fs::path delta_basis_path, fs::path uncompressed_path)
{
	if (!delta_basis_path.empty() && !fs::exists(delta_basis_path))
	{
		std::cout << delta_basis_path.string() << " does not exist." << std::endl;
		throw std::exception();
	}

	std::shared_ptr<std::vector<char>> delta_basis_data;
	if (!delta_basis_path.empty())
	{
		delta_basis_data      = std::make_shared<std::vector<char>>();
		auto delta_basis_size = fs::file_size(delta_basis_path);
		delta_basis_data->resize(delta_basis_size);
		std::ifstream delta_basis(delta_basis_path, std::ios::binary | std::ios::in);
		delta_basis.read(delta_basis_data->data(), delta_basis_size);
	}

	decompress_file(compressed_path, delta_basis_data, uncompressed_path);
}