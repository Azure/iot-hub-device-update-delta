#pragma once

#include <filesystem>

#include <io/reader.h>
#include <io/sequential/writer.h>

#include "cpio_format.h"

namespace archive_diff
{
class cpio_file
{
	public:
	cpio_file() = default;
	cpio_file(const std::string &name, uint32_t inode, cpio_format format, const io::reader &reader);
	cpio_file(cpio_format format);

	static uint64_t get_inode(std::filesystem::path path);

	bool try_read(io::reader &reader, cpio_format *format);
	void write(io::sequential::writer &writer) const;

	bool is_end_marker() const;

	cpio_format get_format() { return m_format; };
	size_t get_total_size() const;

	std::string get_name() const { return m_name; }

	void set_name(const std::string &name)
	{
		m_name     = name;
		m_namesize = static_cast<uint32_t>(name.size()) + 1; // include null terminator
	}

	io::reader get_payload_reader() const { return m_payload_reader; }

	void set_payload_reader(const io::reader &reader);

	private:
	bool try_read_new_ascii_file_entry(const io::reader &reader);

	void regenerate_header_and_padding();

	cpio_format m_format{cpio_format::none};

	std::string m_name;

	uint32_t m_ino{};
	uint32_t m_mode{};
	uint32_t m_uid{};
	uint32_t m_gid{};
	uint32_t m_nlink{};
	uint32_t m_mtime{};
	uint32_t m_filesize{};
	uint32_t m_dev_major{};
	uint32_t m_dev_minor{};
	uint32_t m_rdev_major{};
	uint32_t m_rdev_minor{};
	uint32_t m_namesize{};
	uint32_t m_check{};

	io::reader m_header_reader{};
	io::reader m_header_padding_reader{};
	io::reader m_payload_reader{};
	io::reader m_payload_padding_reader{};
};
} // namespace archive_diff