#pragma once

#include <bsdiff.h>

#include <io/reader.h>
#include <io/writer.h>
#include <io/sequential/writer.h>

namespace archive_diff::io::compressed
{
bsdiff_stream create_reader_based_bsdiff_stream(io::reader &reader);
bsdiff_stream create_writer_based_bsdiff_stream(std::shared_ptr<io::writer> &writer);
bsdiff_stream create_sequential_writer_based_bsdiff_stream(std::shared_ptr<io::sequential::writer> &writer);

class auto_bsdiff_stream : public bsdiff_stream
{
	public:
	auto_bsdiff_stream() = default;
	auto_bsdiff_stream(bsdiff_stream stream) : bsdiff_stream(stream) {}
	auto_bsdiff_stream(const auto_bsdiff_stream &) = delete;
	auto_bsdiff_stream(auto_bsdiff_stream &&other) noexcept : bsdiff_stream(other) { other.state = nullptr; }
	auto_bsdiff_stream &operator=(const auto_bsdiff_stream &) = delete;
	auto_bsdiff_stream &operator=(auto_bsdiff_stream &&other) noexcept
	{
		if (this != &other)
		{
			bsdiff_stream::operator=(other);
			other.state = nullptr;
		}
		return *this;
	}
	virtual ~auto_bsdiff_stream() { close(state); }
};

} // namespace archive_diff::io::compressed
