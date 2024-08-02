/**
 * @file load_ext4.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>

#include <string>
#include <map>
#include <vector>
#include <iterator>
#include <string_view>
#include <fstream>
#include <limits>

extern "C"
{
#include "ext2fs/ext2_types.h"
#include "et/com_err.h"
#include "ext2fs/ext2_io.h"
#include <ext2fs/ext2_ext_attr.h>
#include "ext2fs/ext2fs.h"
}

#ifdef WIN32
	#include <winerror.h>
#endif

static int list_dir_proc(
	ext2_ino_t dir EXT2FS_ATTR((unused)),
	int entry,
	struct ext2_dir_entry *dirent,
	int offset EXT2FS_ATTR((unused)),
	int blocksize EXT2FS_ATTR((unused)),
	char *buf EXT2FS_ATTR((unused)),
	void *priv);

#include "load_ext4.h"
#include "file_details.h"
#include "dump_json.h"

#include <hashing/hasher.h>

struct list_dir_proc_data
{
	io_manager io_ptr;
	ext2_filsys fs;
	const char *parent_dir;
	std::vector<file_details> *all_files_ptr;
};

void get_file_hashes(const char *path, archive_details &details)
{
	std::ifstream input(path, std::ios::binary);

	input.seekg(0, std::ios::end);

	details.length     = input.tellg();
	uint64_t remaining = details.length;

	input.seekg(0, std::ios::beg);

	archive_diff::hashing::hasher sha256_hasher(archive_diff::hashing::algorithm::sha256);
	archive_diff::hashing::hasher md5_hasher(archive_diff::hashing::algorithm::md5);

	const size_t read_block_size = 4 * 1024;
	char buffer[read_block_size];

	while (remaining)
	{
		auto to_read = static_cast<size_t>(std::min<uint64_t>(remaining, read_block_size));

		input.read(buffer, to_read);

		sha256_hasher.hash_data(std::string_view{buffer, to_read});
		md5_hasher.hash_data(std::string_view{buffer, to_read});

		remaining -= to_read;
	}

	details.hash_sha256_string = sha256_hasher.get_hash_string();
	details.hash_md5_string    = md5_hasher.get_hash_string();
}

int load_ext4(const char *ext4_path, archive_details &details)
{
	get_file_hashes(ext4_path, details);

#ifdef WIN32
	io_manager io_ptr = windows_io_manager;
#else
	io_manager io_ptr = unix_io_manager;
#endif
	ext2_filsys fs;
	int retval = ext2fs_open(ext4_path, 0 /* open flags */, 0 /* super block */, 0 /* block size */, io_ptr, &fs);
	if (retval != 0)
	{
		printf("Failed to open ext4 file at: %s. retval: %d\n", ext4_path, retval);
		return retval;
	}

	std::vector<file_details> all_files;

	struct list_dir_proc_data context;
	context.io_ptr        = io_ptr;
	context.fs            = fs;
	context.parent_dir    = "/";
	context.all_files_ptr = &all_files;

	retval = ext2fs_dir_iterate2(fs, EXT2_ROOT_INO, 0, 0, list_dir_proc, (void *)&context);
	if (retval)
	{
		printf("ext2fs_dir_iterate2() failed on EXT2_ROOT_INO. retval: %d\n", retval);
		return retval;
	}

	details.files = std::move(all_files);

	return 0;
}

bool is_all_zeroes(unsigned char *ubuf, unsigned int size)
{
	for (unsigned int i = 0; i < size; i++)
	{
		if (ubuf[i] != 0)
		{
			return false;
		}
	}

	return true;
}

static int populate_file_details_from_inode(ext2_filsys fs, int ino, file_details *details)
{
	struct ext2_inode inode;
	int retval = ext2fs_read_inode(fs, ino, &inode);
	if (retval)
	{
		printf("ext2fs_read_inode() failed: %d\n", retval);
		return retval;
	}

	if (inode.i_flags & EXT4_INLINE_DATA_FL)
	{
		// TODO: Implement handling of inline data?
		// May not be worth it since these files are small and the diff won't
		// be very valuable
		return 0;
	}

	if (ext2fs_is_fast_symlink(&inode))
	{
		return 0;
	}

	__u64 file_size        = EXT2_I_SIZE(&inode);
	__u64 read             = 0;
	details->length        = file_size;
	unsigned int blocksize = fs->blocksize;
	unsigned int got;
	bool current_block_all_zeroes{true};

	ext2_file_t e2_file;
	retval = ext2fs_file_open(fs, ino, 0, &e2_file);
	if (retval)
	{
		printf("ext2fs_file_open() failed: %d\n", retval);
		return retval;
	}

	std::vector<char> buf;
	buf.reserve(blocksize);
	blk64_t prev_physblock = std::numeric_limits<blk64_t>::max();
	__u64 offset           = std::numeric_limits<__u64>::max();
	__u64 length           = 0;

	archive_diff::hashing::hasher hasher_md5(archive_diff::hashing::algorithm::md5);
	archive_diff::hashing::hasher hasher_sha256(archive_diff::hashing::algorithm::sha256);

	archive_diff::hashing::hasher hasher_md5_region(archive_diff::hashing::algorithm::md5);
	archive_diff::hashing::hasher hasher_sha256_region(archive_diff::hashing::algorithm::sha256);

	while (read < file_size)
	{
		retval = ext2fs_file_read(e2_file, const_cast<char *>(buf.data()), blocksize, &got);
#ifdef WIN32
		if (retval == ERROR_NEGATIVE_SEEK)
		{
			// Loading directories from windows file system sometimes fails with a negative seek
			return 0;
		}
#endif
		if (retval == EXT2_ET_SHORT_READ)
		{
			// Very small files are probably not worth diffing
			return 0;
		}
		else if (retval)
		{
			printf("ext2fs_file_read() failed: %d\n", retval);
			return retval;
		}

		unsigned char *ubuf = reinterpret_cast<unsigned char *>(buf.data());

		hasher_md5.hash_data(ubuf, got);
		hasher_sha256.hash_data(ubuf, got);

		blk64_t current_physblock = ext2fs_file_get_current_physblock(e2_file);

		bool no_valid_previous = prev_physblock == std::numeric_limits<blk64_t>::max();

		if (no_valid_previous || (current_physblock != (prev_physblock + 1)))
		{
			if (length)
			{
				auto offset_value = current_block_all_zeroes ? std::nullopt : std::optional<uint64_t>{offset};

				details->regions.emplace_back(file_region{
					offset_value,
					length,
					current_block_all_zeroes,
					hasher_md5_region.get_hash_string(),
					hasher_sha256_region.get_hash_string()});

				hasher_md5_region.reset();
				hasher_sha256_region.reset();
			}
			offset                   = blocksize * current_physblock;
			length                   = 0;
			current_block_all_zeroes = true;
		}

		hasher_md5_region.hash_data(ubuf, got);
		hasher_sha256_region.hash_data(ubuf, got);

		if (current_block_all_zeroes)
		{
			current_block_all_zeroes = is_all_zeroes(ubuf, got);
		}

		length += got;
		prev_physblock = current_physblock;

		read += got;
	}

	if (length)
	{
		auto offset_value = current_block_all_zeroes ? std::nullopt : std::optional<uint64_t>{offset};

		details->regions.emplace_back(file_region{
			offset_value,
			length,
			current_block_all_zeroes,
			hasher_md5_region.get_hash_string(),
			hasher_sha256_region.get_hash_string()});
	}

	details->hash_md5_string    = hasher_md5.get_hash_string();
	details->hash_sha256_string = hasher_sha256.get_hash_string();

	return 0;
}

static int list_dir_proc(
	[[maybe_unused]] ext2_ino_t dir EXT2FS_ATTR((unused)),
	int entry,
	struct ext2_dir_entry *dirent,
	[[maybe_unused]] int offset EXT2FS_ATTR((unused)),
	[[maybe_unused]] int blocksize EXT2FS_ATTR((unused)),
	[[maybe_unused]] char *buf EXT2FS_ATTR((unused)),
	void *priv)
{
	if (entry != DIRENT_OTHER_FILE)
	{
		return 0;
	}

	int name_len = ext2fs_dirent_name_len(dirent);

	std::string name(dirent->name, name_len);

	struct list_dir_proc_data *context = reinterpret_cast<struct list_dir_proc_data *>(priv);
	ext2_filsys fs                     = context->fs;
	auto all_files_ptr                 = context->all_files_ptr;

	std::string full_path = context->parent_dir;
	if (full_path.back() != '/')
	{
		full_path += "/";
	}
	full_path += name;

	int retval = ext2fs_check_directory(fs, dirent->inode);
	if (retval == EXT2_ET_NO_DIRECTORY)
	{
		file_details details;
		details.full_path = full_path;
		details.name      = name;
		retval            = populate_file_details_from_inode(fs, dirent->inode, &details);
		if (retval)
		{
			printf("populate_file_details_from_inode() for %s failed: %d\n", full_path.c_str(), retval);
			return retval;
		}
		if (details.regions.size() > 0)
		{
			all_files_ptr->emplace_back(std::move(details));
		}

		return 0;
	}
#ifdef WIN32
	else if (retval == ERROR_NEGATIVE_SEEK)
	{
		return 0;
	}
#endif
	else if (retval)
	{
		printf("ext2fs_check_directory() failed: %d\n", retval);
		return retval;
	}

	struct list_dir_proc_data context_new;
	context_new            = *context;
	context_new.parent_dir = full_path.c_str();

	retval = ext2fs_dir_iterate2(fs, dirent->inode, 0, 0, list_dir_proc, (void *)&context_new);
	if (retval)
	{
		printf("ext2fs_dir_iterate2() failed: %d\n", retval);
		return retval;
	}

	return 0;
}
