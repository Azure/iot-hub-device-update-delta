#include "cpio_archive.h"

#include <io/reader.h>
#include <io/sequential/writer.h>

#include <string_view>

namespace archive_diff
{
bool cpio_archive::has_file(const std::string &name) const { return get_file_index(name) != m_files.size(); }

size_t cpio_archive::get_file_index(const std::string &name) const
{
	auto itr = std::find_if(
		std::begin(m_files),
		std::end(m_files),
		[&](const cpio_file &file)
		{
			auto this_name = file.get_name();
			return (name.compare(name) == 0);
		});

	return std::distance(std::begin(m_files), itr);
}

void cpio_archive::remove_file(const std::string &to_remove)
{
	std::erase_if(
		m_files,
		[&](const cpio_file &file)
		{
			auto name = file.get_name();
			return (name.compare(to_remove) == 0);
		});

	m_payload_reader_map.erase(to_remove);
}

void cpio_archive::add_file(const std::string &to_add, uint32_t inode, const io::reader &reader)
{
	m_files.emplace_back(cpio_file{to_add, inode, m_format, reader});
}

void cpio_archive::insert_file_at(size_t index, const std::string &to_add, uint32_t inode, const io::reader &reader)
{
	m_files.insert(m_files.begin() + index, cpio_file{to_add, inode, m_format, reader});
}

bool cpio_archive::try_read(io::reader &reader, cpio_format format)
{
	uint64_t offset = 0;

	std::vector<cpio_file> files;

	cpio_format actual_format   = format;
	cpio_format previous_format = cpio_format::none;

	while (true)
	{
		auto slice = reader.slice_at(offset);

		cpio_file file;

		if (!file.try_read(slice, &actual_format))
		{
			return false;
		}

		// if we've already in a file, make sure that we're not mixing files of different formats
		if (!files.empty())
		{
			if (previous_format != actual_format)
			{
				return false;
			}
		}

		previous_format = actual_format;

		offset += file.get_total_size();

		if (file.is_end_marker())
		{
			break;
		}

		auto name                  = file.get_name();
		m_payload_reader_map[name] = file.get_payload_reader();

		files.emplace_back(std::move(file));
	}

	m_files  = std::move(files);
	m_format = actual_format;
	return true;
}

bool cpio_archive::try_read(io::reader &reader)
{
	const cpio_format formats[] = {cpio_format::new_ascii, cpio_format::ascii, cpio_format::binary};

	for (const auto &format : formats)
	{
		if (try_read(reader, format))
		{
			return true;
		}
	}

	return false;
}

io::reader cpio_archive::get_payload_reader(const std::string &name)
{
	auto ret = m_payload_reader_map.find(name);
	if (ret != m_payload_reader_map.cend())
	{
		return ret->second;
	}

	return io::reader{};
}

void cpio_archive::set_payload_reader(const std::string &filename, io::reader &reader)
{
	bool found = false;
	for (auto &file : m_files)
	{
		auto name = file.get_name();
		if (name.compare(filename) != 0)
		{
			continue;
		}

		file.set_payload_reader(reader);
		found = true;
	}

	if (!found)
	{
		return;
	}

	m_payload_reader_map[filename] = reader;
}

void cpio_archive::write(io::sequential::writer &writer)
{
	for (const auto &file : m_files)
	{
		file.write(writer);
	}

	cpio_file trailer(m_format);
	trailer.write(writer);
}
} // namespace archive_diff