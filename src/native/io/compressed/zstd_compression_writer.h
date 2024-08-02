/**
 * @file zstd_compression_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>

#include <zstd.h>

#include <io/writer.h>
#include <io/sequential/writer_impl.h>

#include "zstd_wrappers.h"

#include "compression_dictionary.h"

namespace archive_diff::io::compressed
{
// We could potentially omit the uncompressed_size, but then we wouldn't
// know when we're done. This would mean we wouldn't set
// ZSTD_EndDirective::ZSTD_e_end, which would produce decompressable content,
// but the last block would be different from if we had passed this value -
// since we want deterministic content, this is bad.
// We would also need some other way to determine we're done, such as
// repurposing flush or adding a new API which would make this writer
// divergent from others in its behavior.
class zstd_compression_writer : public io::sequential::writer_impl
{
	public:
	zstd_compression_writer(
		std::shared_ptr<io::sequential::writer> &writer, uint64_t level, uint64_t uncompressed_input_size);

	zstd_compression_writer(
		std::shared_ptr<io::sequential::writer> &writer,
		uint64_t level,
		uint64_t uncompressed_input_size,
		compression_dictionary &&dictionary);

	virtual ~zstd_compression_writer() = default;

	virtual uint64_t tellp() override { return m_processed_bytes; }
	virtual void flush() override;

	protected:
	virtual void write_impl(std::string_view buffer) override;

	private:
	void set_dictionary(const compression_dictionary &dictionary);

	std::shared_ptr<io::sequential::writer> m_writer{};
	uint64_t m_uncompressed_input_size{};
	uint64_t m_compressed_size{};
	compression_dictionary m_compression_dictionary;

	uint64_t m_processed_bytes{};

	unique_zstd_cstream m_zstd_cstream{ZSTD_createCStream()};

	ZSTD_inBuffer m_input_buffer{};
	ZSTD_outBuffer m_output_buffer{};

	std::vector<char> m_input_data;
	std::vector<char> m_output_data;
};
}; // namespace archive_diff::io::compressed
