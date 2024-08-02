/**
 * @file zstd_decompression_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>

#include <zstd.h>

#include <io/writer.h>
#include <io/sequential/writer.h>
#include <io/sequential/writer_impl.h>

#include "zstd_wrappers.h"

#include "compression_dictionary.h"

namespace archive_diff::io::compressed
{
class zstd_decompression_writer : public io::sequential::writer_impl
{
	public:
	zstd_decompression_writer(std::shared_ptr<io::sequential::writer> &writer);
	zstd_decompression_writer(std::shared_ptr<io::sequential::writer> &writer, compression_dictionary &&dictionary);
	virtual ~zstd_decompression_writer();

	virtual uint64_t tellp() override { return m_processed_bytes; }
	virtual void flush() override;

	protected:
	virtual void write_impl(std::string_view buffer);

	private:
	compression_dictionary m_compression_dictionary;

	uint64_t m_processed_bytes{};

	std::shared_ptr<io::sequential::writer> m_writer{};
	unique_zstd_dstream m_zstd_dstream{ZSTD_createDStream()};

	ZSTD_inBuffer m_input_buffer{};
	ZSTD_outBuffer m_output_buffer{};

	std::vector<char> m_input_data;
	std::vector<char> m_output_data;
};
}; // namespace archive_diff::io::compressed
