/**
 * @file zlib_decompression_reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/sequential/reader_factory.h>
#include <io/compressed/zlib_decompression_reader.h>

#include <diffs/core/item_definition.h>
#include <diffs/core/prepared_item.h>

namespace archive_diff::diffs::core
{
class zlib_decompression_reader_factory : public io::sequential::reader_factory
{
	using decompression_reader = io::compressed::zlib_decompression_reader;
	using init_type            = io::compressed::zlib_helpers::init_type;
	using item_definition      = diffs::core::item_definition;
	using prepared_item        = diffs::core::prepared_item;

	public:
	zlib_decompression_reader_factory(
		const item_definition &uncompressed,
		std::shared_ptr<prepared_item> &compressed_prepared_item,
		init_type init_type) :
		m_uncompressed_result(uncompressed), m_compressed_prepared_item(compressed_prepared_item),
		m_init_type(init_type)
	{}

	virtual std::unique_ptr<io::sequential::reader> make_sequential_reader() override
	{
		auto compressed_reader = m_compressed_prepared_item->make_sequential_reader();

		return std::make_unique<decompression_reader>(
			std::move(compressed_reader), m_uncompressed_result.size(), m_init_type);
	}

	private:
	item_definition m_uncompressed_result{};
	std::shared_ptr<prepared_item> m_compressed_prepared_item{};
	init_type m_init_type;
};
} // namespace archive_diff::diffs::core