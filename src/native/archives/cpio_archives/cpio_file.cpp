#include "cpio_file.h"

#include "octal_data.h"
#include "hexadecimal_data.h"
#include "string_data.h"

#include <io/buffer/io_device.h>
#include <io/all_zeros_io_device.h>
#include <io/sequential/basic_reader_wrapper.h>

#ifdef WIN32
	#include <windows.h>
#else
	#include <sys/stat.h>
#endif

#include <iostream>
#include <array>

namespace archive_diff
{
uint32_t compute_newc_checksum(const io::reader &reader);

const char *TRAILER_NAME = "TRAILER!!!";

cpio_file::cpio_file(const std::string &name, uint32_t inode, cpio_format format, const io::reader &reader) :
	m_format(format)
{
	set_name(name);

	m_ino       = inode;
	m_uid       = 1000;
	m_gid       = 1000;
	m_nlink     = 1;
	m_dev_major = 8;
	m_dev_minor = 1;

	set_payload_reader(reader);
}

cpio_file::cpio_file(cpio_format format) : m_format(format)
{
	set_name(TRAILER_NAME);
	m_nlink = 1;

	auto empty_reader = io::reader();
	set_payload_reader(empty_reader);
}

void cpio_file::write(io::sequential::writer &writer) const
{
	writer.write(m_header_reader);
	writer.write(m_header_padding_reader);
	writer.write(m_payload_reader);
	writer.write(m_payload_padding_reader);
}

bool cpio_file::is_end_marker() const { return (m_name.compare(TRAILER_NAME) == 0); }

size_t cpio_file::get_total_size() const
{
	return static_cast<size_t>(
		m_header_reader.size() + m_header_padding_reader.size() + m_payload_reader.size()
		+ m_payload_padding_reader.size());
}

bool cpio_file::try_read(io::reader &reader, cpio_format *format)
{
	if (format == nullptr)
	{
		throw std::exception();
	}

	switch (*format)
	{
	case cpio_format::new_ascii:
	case cpio_format::newc_ascii:
		if (try_read_new_ascii_file_entry(reader))
		{
			*format = m_format;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

const char NEW_ASCII_MAGIC[6]      = {'0', '7', '0', '7', '0', '1'};
const char NEWC_ASCII_MAGIC[6]     = {'0', '7', '0', '7', '0', '2'};
const size_t NEW_ASCII_HEADER_SIZE = 110;

const size_t NEW_ASCII_HEADER_MAGIC_OFFSET      = 0;
const size_t NEW_ASCII_HEADER_INO_OFFSET        = 6;
const size_t NEW_ASCII_HEADER_MODE_OFFSET       = 14;
const size_t NEW_ASCII_HEADER_UID_OFFSET        = 22;
const size_t NEW_ASCII_HEADER_GID_OFFSET        = 30;
const size_t NEW_ASCII_HEADER_NLINK_OFFSET      = 38;
const size_t NEW_ASCII_HEADER_MTIME_OFFSET      = 46;
const size_t NEW_ASCII_HEADER_FILESIZE_OFFSET   = 54;
const size_t NEW_ASCII_HEADER_DEV_MAJOR_OFFSET  = 62;
const size_t NEW_ASCII_HEADER_DEV_MINOR_OFFSET  = 70;
const size_t NEW_ASCII_HEADER_RDEV_MAJOR_OFFSET = 78;
const size_t NEW_ASCII_HEADER_RDEV_MINOR_OFFSET = 86;
const size_t NEW_ASCII_HEADER_NAMESIZE_OFFSET   = 94;
const size_t NEW_ASCII_HEADER_CHECK_OFFSET      = 102;

const size_t NEW_ASCII_HEADER_NAME_OFFSET = NEW_ASCII_HEADER_SIZE;

const size_t NEW_ASCII_HEADER_MAGIC_LENGTH      = 6;
const size_t NEW_ASCII_HEADER_INO_LENGTH        = 8;
const size_t NEW_ASCII_HEADER_MODE_LENGTH       = 8;
const size_t NEW_ASCII_HEADER_UID_LENGTH        = 8;
const size_t NEW_ASCII_HEADER_GID_LENGTH        = 8;
const size_t NEW_ASCII_HEADER_NLINK_LENGTH      = 8;
const size_t NEW_ASCII_HEADER_MTIME_LENGTH      = 8;
const size_t NEW_ASCII_HEADER_FILESIZE_LENGTH   = 8;
const size_t NEW_ASCII_HEADER_DEV_MAJOR_LENGTH  = 8;
const size_t NEW_ASCII_HEADER_DEV_MINOR_LENGTH  = 8;
const size_t NEW_ASCII_HEADER_RDEV_MAJOR_LENGTH = 8;
const size_t NEW_ASCII_HEADER_RDEV_MINOR_LENGTH = 8;
const size_t NEW_ASCII_HEADER_NAMESIZE_LENGTH   = 8;
const size_t NEW_ASCII_HEADER_CHECK_LENGTH      = 8;

struct cpio_newc_header
{
	char c_magic[6];
	char c_ino[8];
	char c_mode[8];
	char c_uid[8];
	char c_gid[8];
	char c_nlink[8];
	char c_mtime[8];
	char c_filesize[8];
	char c_devmajor[8];
	char c_devminor[8];
	char c_rdevmajor[8];
	char c_rdevminor[8];
	char c_namesize[8];
	char c_check[8];
};

size_t get_padding_needed(size_t offset, size_t alignment)
{
	if (alignment == 0)
	{
		return 0;
	}

	size_t padding_needed = alignment - (offset % alignment);
	if (padding_needed == alignment)
	{
		return 0;
	}

	return padding_needed;
}

void cpio_file::regenerate_header_and_padding()
{
	switch (m_format)
	{
	case cpio_format::new_ascii:
	case cpio_format::newc_ascii: {
		auto header_data         = std::make_shared<std::vector<char>>();
		size_t total_header_size = NEW_ASCII_HEADER_SIZE;

		total_header_size += m_namesize;

		header_data->resize(total_header_size);

		struct
		{
			uint32_t *value;
			size_t offset;
			size_t length;
		} values[] = {
			{&m_ino, NEW_ASCII_HEADER_INO_OFFSET, NEW_ASCII_HEADER_INO_LENGTH},
			{&m_mode, NEW_ASCII_HEADER_MODE_OFFSET, NEW_ASCII_HEADER_MODE_LENGTH},
			{&m_uid, NEW_ASCII_HEADER_UID_OFFSET, NEW_ASCII_HEADER_UID_LENGTH},
			{&m_gid, NEW_ASCII_HEADER_GID_OFFSET, NEW_ASCII_HEADER_GID_LENGTH},
			{&m_nlink, NEW_ASCII_HEADER_NLINK_OFFSET, NEW_ASCII_HEADER_NLINK_LENGTH},
			{&m_mtime, NEW_ASCII_HEADER_MTIME_OFFSET, NEW_ASCII_HEADER_MTIME_LENGTH},
			{&m_filesize, NEW_ASCII_HEADER_FILESIZE_OFFSET, NEW_ASCII_HEADER_FILESIZE_LENGTH},
			{&m_dev_major, NEW_ASCII_HEADER_DEV_MAJOR_OFFSET, NEW_ASCII_HEADER_DEV_MAJOR_LENGTH},
			{&m_dev_minor, NEW_ASCII_HEADER_DEV_MINOR_OFFSET, NEW_ASCII_HEADER_DEV_MINOR_LENGTH},
			{&m_rdev_major, NEW_ASCII_HEADER_RDEV_MAJOR_OFFSET, NEW_ASCII_HEADER_RDEV_MAJOR_LENGTH},
			{&m_rdev_minor, NEW_ASCII_HEADER_RDEV_MINOR_OFFSET, NEW_ASCII_HEADER_RDEV_MINOR_LENGTH},
			{&m_namesize, NEW_ASCII_HEADER_NAMESIZE_OFFSET, NEW_ASCII_HEADER_NAMESIZE_LENGTH},
			{&m_check, NEW_ASCII_HEADER_CHECK_OFFSET, NEW_ASCII_HEADER_CHECK_LENGTH},
		};

		auto magic = (m_format == cpio_format::new_ascii) ? NEW_ASCII_MAGIC : NEWC_ASCII_MAGIC;

		memcpy(header_data->data(), magic, sizeof(NEWC_ASCII_MAGIC));

		for (const auto &value : values)
		{
			uint32_to_hexadecimal_characters(
				*value.value, std::span<char>{header_data->data() + value.offset, value.length});
		}

		memcpy(header_data->data() + NEW_ASCII_HEADER_SIZE, m_name.c_str(), m_name.size());

		m_header_reader =
			io::buffer::io_device::make_reader(header_data, io::buffer::io_device::size_kind::vector_size);

		auto header_padding_size = get_padding_needed(total_header_size, 4);
		m_header_padding_reader  = io::all_zeros_io_device::make_reader(header_padding_size);

		auto payload_padding_size = get_padding_needed(static_cast<size_t>(m_payload_reader.size()), 4);
		m_payload_padding_reader  = io::all_zeros_io_device::make_reader(payload_padding_size);
	}
	break;
	default:
		throw std::exception();
	}
}

uint32_t compute_newc_checksum(const io::reader &reader)
{
	uint32_t sum = 0;

	io::sequential::basic_reader_wrapper wrapper(reader);

	const size_t block_size = 8 * 1024;
	std::array<char, block_size> buffer{};

	while (true)
	{
		auto actual_read = wrapper.read_some(std::span<char>{buffer.data(), buffer.size()});

		if (actual_read == 0)
		{
			break;
		}

		for (uint64_t i = 0; i < actual_read; i++)
		{
			sum += static_cast<unsigned char>(buffer[i]);
		}
	}

	return sum;
}

void cpio_file::set_payload_reader(const io::reader &reader)
{
	m_payload_reader = reader;

	m_filesize = static_cast<uint32_t>(reader.size());
	if (m_format == cpio_format::newc_ascii)
	{
		m_check = compute_newc_checksum(reader);
	}

	regenerate_header_and_padding();
}

bool cpio_file::try_read_new_ascii_file_entry(const io::reader &reader)
{
	char header[NEW_ASCII_HEADER_SIZE];
	cpio_format format;

	auto header_amount_read = reader.read_some(0, std::span<char>{header, sizeof(header)});
	if (header_amount_read != sizeof(header))
	{
		return false;
	}

	if (0 == memcmp(header, NEW_ASCII_MAGIC, sizeof(NEW_ASCII_MAGIC)))
	{
		format = cpio_format::new_ascii;
	}
	else if (0 == memcmp(header, NEWC_ASCII_MAGIC, sizeof(NEWC_ASCII_MAGIC)))
	{
		format = cpio_format::newc_ascii;
	}
	else
	{
		return false;
	}

	struct
	{
		uint32_t *value;
		size_t offset;
		size_t length;
	} values[] = {
		{&m_ino, NEW_ASCII_HEADER_INO_OFFSET, NEW_ASCII_HEADER_INO_LENGTH},
		{&m_mode, NEW_ASCII_HEADER_MODE_OFFSET, NEW_ASCII_HEADER_MODE_LENGTH},
		{&m_uid, NEW_ASCII_HEADER_UID_OFFSET, NEW_ASCII_HEADER_UID_LENGTH},
		{&m_gid, NEW_ASCII_HEADER_GID_OFFSET, NEW_ASCII_HEADER_GID_LENGTH},
		{&m_nlink, NEW_ASCII_HEADER_NLINK_OFFSET, NEW_ASCII_HEADER_NLINK_LENGTH},
		{&m_mtime, NEW_ASCII_HEADER_MTIME_OFFSET, NEW_ASCII_HEADER_MTIME_LENGTH},
		{&m_filesize, NEW_ASCII_HEADER_FILESIZE_OFFSET, NEW_ASCII_HEADER_FILESIZE_LENGTH},
		{&m_dev_major, NEW_ASCII_HEADER_DEV_MAJOR_OFFSET, NEW_ASCII_HEADER_DEV_MAJOR_LENGTH},
		{&m_dev_minor, NEW_ASCII_HEADER_DEV_MINOR_OFFSET, NEW_ASCII_HEADER_DEV_MINOR_LENGTH},
		{&m_rdev_major, NEW_ASCII_HEADER_RDEV_MAJOR_OFFSET, NEW_ASCII_HEADER_RDEV_MAJOR_LENGTH},
		{&m_rdev_minor, NEW_ASCII_HEADER_RDEV_MINOR_OFFSET, NEW_ASCII_HEADER_RDEV_MINOR_LENGTH},
		{&m_namesize, NEW_ASCII_HEADER_NAMESIZE_OFFSET, NEW_ASCII_HEADER_NAMESIZE_LENGTH},
		{&m_check, NEW_ASCII_HEADER_CHECK_OFFSET, NEW_ASCII_HEADER_CHECK_LENGTH},
	};

	for (const auto &value : values)
	{
		*value.value = hexadecimal_characters_to_uint32(header, value.offset, value.length);
	}

	std::vector<char> name_buffer;
	name_buffer.reserve(m_namesize);

	if (reader.read_some(NEW_ASCII_HEADER_NAME_OFFSET, std::span<char>{name_buffer.data(), m_namesize}) != m_namesize)
	{
		return false;
	}

	m_name = string_from_nul_padded_string(std::string_view{name_buffer.data(), m_namesize});

	size_t header_total_size   = NEW_ASCII_HEADER_SIZE + m_namesize;
	size_t header_padding_size = get_padding_needed(header_total_size, 4);

	size_t file_data_start_offset = header_total_size + header_padding_size;
	size_t file_data_end_offset   = file_data_start_offset + m_filesize;
	size_t payload_padding_size   = get_padding_needed(file_data_end_offset, 4);

	m_header_reader         = reader.slice(0, header_total_size);
	m_header_padding_reader = reader.slice(header_total_size, header_padding_size);
	m_payload_reader        = reader.slice(file_data_start_offset, m_filesize);

	// if this is the end, just interpret the rest of the file as padding.
	if (is_end_marker())
	{
		m_payload_padding_reader = reader.slice_at(file_data_end_offset);
	}
	else
	{
		m_payload_padding_reader = reader.slice(file_data_end_offset, payload_padding_size);
	}

	m_format = format;

	return true;
}

uint64_t cpio_file::get_inode(std::filesystem::path path)
{
#ifdef WIN32
	HANDLE hFile = CreateFileA(
		path.string().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	BY_HANDLE_FILE_INFORMATION file_info;
	if (!GetFileInformationByHandle(hFile, &file_info))
	{
		throw std::exception();
	}

	ULARGE_INTEGER li{file_info.nFileIndexLow, file_info.nFileIndexHigh};

	CloseHandle(hFile);

	return li.QuadPart;
#else
	struct stat file_info;
	if (!stat(path.string().c_str(), &file_info))
	{
		throw std::exception();
	}

	return static_cast<uint64_t>(file_info.st_ino);
#endif
}

} // namespace archive_diff