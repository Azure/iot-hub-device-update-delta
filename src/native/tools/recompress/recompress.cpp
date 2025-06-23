/**
 * @file recompress.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <string>

#include <vector>
#include <map>
#include <set>
#include <memory>

#include <language_support/include_filesystem.h>
#include <io/sequential/basic_writer_wrapper.h>
#include <io/sequential/basic_reader_wrapper.h>
#include <io/compressed/zlib_decompression_reader.h>

#include <io/compressed/zlib_compression_reader.h>
#include <io/compressed/zlib_decompression_writer.h>
#include <io/compressed/zlib_compression_writer.h>

#include <hashing/hasher.h>

#include <io/file/binary_file_writer.h>
#include <io/file/io_device.h>

void recompress(archive_diff::io::reader &reader, std::shared_ptr<archive_diff::io::writer> &writer)
{
	auto wrapper = archive_diff::io::sequential::basic_writer_wrapper::make_shared(writer);
	std::shared_ptr<archive_diff::io::sequential::writer> compressor =
		std::make_shared<archive_diff::io::compressed::zlib_compression_writer>(
			wrapper, Z_BEST_COMPRESSION, archive_diff::io::compressed::zlib_init_type::gz);
	archive_diff::io::compressed::zlib_decompression_writer decompressor(
		compressor, archive_diff::io::compressed::zlib_init_type::gz);

	decompressor.write(reader);
}

void recompress(fs::path &source, fs::path &dest)
{
	printf("Recompressing %s into %s\n", source.string().c_str(), dest.string().c_str());

	auto reader = archive_diff::io::file::io_device::make_reader(source.string());

	printf("Writing recompressed data to: %s\n", dest.string().c_str());

	std::shared_ptr<archive_diff::io::writer> file_writer =
		std::make_shared<archive_diff::io::file::binary_file_writer>(dest.string());

	recompress(reader, file_writer);
}

fs::path recompress_using_reader(fs::path &source, fs::path &dest)
{
	auto uncompressed_dest = dest;
	uncompressed_dest.replace_extension("");

	{
		printf("Uncompressing %s into %s\n", source.string().c_str(), uncompressed_dest.string().c_str());

		auto reader = archive_diff::io::file::io_device::make_reader(source.string());

		std::shared_ptr<archive_diff::io::writer> file_writer =
			std::make_shared<archive_diff::io::file::binary_file_writer>(uncompressed_dest.string());

		auto wrapper = archive_diff::io::sequential::basic_writer_wrapper::make_shared(file_writer);
		archive_diff::io::compressed::zlib_decompression_writer decompressor(
			wrapper, archive_diff::io::compressed::zlib_init_type::gz);

		decompressor.write(reader);
	}

	auto dest2 = dest;
	dest2.replace_extension(".2");

	{
		printf("Writing compressed data to: %s\n", dest2.string().c_str());

		auto reader = archive_diff::io::file::io_device::make_reader(uncompressed_dest.string());

		auto uncompressed_size = fs::file_size(uncompressed_dest);
		auto compressed_size   = fs::file_size(dest);
		archive_diff::io::compressed::zlib_compression_reader compressor(
			reader, Z_BEST_SPEED, uncompressed_size, compressed_size, archive_diff::io::compressed::zlib_init_type::gz);

		std::shared_ptr<archive_diff::io::writer> file_writer =
			std::make_shared<archive_diff::io::file::binary_file_writer>(dest2.string());

		archive_diff::io::sequential::basic_writer_wrapper seq(file_writer);

		seq.write(compressor);
	}

	return dest2;
}

bool compare_file_hashes(fs::path &file1, fs::path &file2, const std::string &hash1, const std::string &hash2)
{
	if (hash1.size() != hash2.size())
	{
		printf("Mismatch between hash sizes for %s and %s.\n", file1.string().c_str(), file2.string().c_str());
		return false;
	}

	if (hash1.compare(hash2) != 0)
	{
		printf(
			"Mismatch hashes for %s and %s. Hash 1: %s, Hash2: %s\n",
			file1.string().c_str(),
			file2.string().c_str(),
			hash1.c_str(),
			hash2.c_str());
		return false;
	}

	printf("%s and %s have matching content.\n", file1.string().c_str(), file2.string().c_str());
	return true;
}

std::string get_filehash_string(fs::path &path)
{
	archive_diff::hashing::hasher hasher(archive_diff::hashing::algorithm::sha256);

	auto reader = archive_diff::io::file::io_device::make_reader(path.string());
	archive_diff::io::sequential::basic_reader_wrapper seq(reader);

	const uint64_t block_size = 32 * 1024;
	std::vector<char> buffer;
	buffer.reserve(block_size);

	auto remaining = seq.size();
	while (remaining > 0)
	{
		auto actual_read = seq.read_some(std::span<char>{buffer.data(), buffer.capacity()});

		hasher.hash_data(std::string_view{buffer.data(), actual_read});

		remaining -= actual_read;
	}

	return hasher.get_hash_string();
}