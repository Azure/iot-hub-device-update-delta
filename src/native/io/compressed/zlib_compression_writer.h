/**
 * @file zlib_compression_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include "zlib_helpers.h"

#include <io/writer.h>
#include <io/reader.h>

#include <io/sequential/writer_impl.h>

namespace archive_diff::io::compressed
{
// We require the uncompressed_size, because we need to know when to
//
// We would also need some other way to determine we're done, such as
// repurposing flush or adding a new API which would make this writer
// divergent from others in its behavior.
class zlib_compression_writer : public io::sequential::writer_impl
{
	public:
	zlib_compression_writer(
		std::shared_ptr<io::sequential::writer> &writer, int level, zlib_helpers::init_type init_type);

	virtual ~zlib_compression_writer() { flush(); }

	virtual uint64_t tellp() override { return m_processed_bytes; }
	virtual void flush() override;

	protected:
	virtual void write_impl(std::string_view buffer) override;

	private:
	void reset_output();
	void handle_output();
	void stream_end();

	uint64_t m_compressed_output_size{};
	uint64_t m_processed_bytes{};

	std::shared_ptr<io::sequential::writer> m_writer{};

	int m_compression_level{};
	zlib_helpers::init_type m_init_type{};

	auto_zlib_cstream m_zstr{};

	std::vector<char> m_output_data;
};
}; // namespace archive_diff::io::compressed
