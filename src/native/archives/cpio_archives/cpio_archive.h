#include <map>

#include <io/reader.h>
#include <io/sequential/writer.h>

#include "cpio_format.h"
#include "cpio_file.h"

namespace archive_diff
{
class cpio_archive
{
	public:
	cpio_archive() = default;
	cpio_archive(cpio_format format) { m_format = format; }

	bool try_read(io::reader &reader);
	void write(io::sequential::writer &writer);

	cpio_format get_format() { return m_format; };
	std::vector<std::string> get_filenames() const
	{
		std::vector<std::string> names;

		for (const auto &file : m_files)
		{
			names.emplace_back(file.get_name());
		}

		return names;
	}

	bool has_file(const std::string &name) const;
	void remove_file(const std::string &to_remove);

	size_t get_file_index(const std::string &name) const;

	void add_file(const std::string &to_add, uint32_t inode, const io::reader &reader);
	void insert_file_at(size_t index, const std::string &to_add, uint32_t inode, const io::reader &reader);

	io::reader get_payload_reader(const std::string &name);
	void set_payload_reader(const std::string &filename, io::reader &reader);

	private:
	bool try_read(io::reader &reader, cpio_format format);
	cpio_format m_format{cpio_format::none};

	std::vector<cpio_file> m_files;
	std::map<std::string, io::reader> m_payload_reader_map;
};
} // namespace archive_diff