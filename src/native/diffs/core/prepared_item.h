/**
 * @file prepared_item.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/basic_reader_factory.h>
#include <io/reader_factory.h>
#include <io/sequential/reader_factory.h>

#include "item_definition.h"

#include <variant>

namespace archive_diff::diffs::core
{
class kitchen;

class prepared_item
{
	public:
	struct reader_kind
	{
		std::shared_ptr<io::reader_factory> m_factory;
		std::vector<std::shared_ptr<prepared_item>> m_ingredients;
	};

	struct sequential_reader_kind
	{
		std::shared_ptr<io::sequential::reader_factory> m_factory;
		std::vector<std::shared_ptr<prepared_item>> m_ingredients;
	};

	struct slice_kind
	{
		uint64_t m_offset{};
		uint64_t m_length{};

		std::shared_ptr<prepared_item> m_item;
	};

	struct chain_kind
	{
		std::vector<std::shared_ptr<prepared_item>> m_items;
	};

	struct fetch_slice_kind
	{
		std::weak_ptr<diffs::core::kitchen> kitchen{};
	};

	prepared_item(const item_definition &item_definition, const reader_kind &reader) :
		m_item_definition(item_definition), m_kind(reader)
	{}

	prepared_item(const item_definition &item_definition, io::reader &reader) :
		m_item_definition(item_definition), m_kind(reader_kind{std::make_shared<io::basic_reader_factory>(reader)})
	{}

	prepared_item(const item_definition &item_definition, const sequential_reader_kind &sequential_reader) :
		m_item_definition(item_definition), m_kind(sequential_reader)
	{}

	prepared_item(const item_definition &item_definition, const slice_kind &slice) :
		m_item_definition(item_definition), m_kind(slice)
	{}

	prepared_item(const item_definition &item_definition, const chain_kind &chain) :
		m_item_definition(item_definition), m_kind(chain)
	{}

	prepared_item(const item_definition &item_definition, const fetch_slice_kind &fetch) :
		m_item_definition(item_definition), m_kind(fetch)
	{}

	const item_definition &get_item_definition() const { return m_item_definition; }
	const uint64_t size() const { return m_item_definition.size(); }

	// We can't make a reader out of content composed of sequential readers (individual,
	// chains or mixed chains of sequential/random
	bool can_make_reader() const;
	io::reader make_reader();

	void write(std::shared_ptr<io::writer> &writer);

	// If we could construct this prepared item, we should always be able to create a sequential reader
	std::unique_ptr<io::sequential::reader> make_sequential_reader() const;

	bool can_slice(uint64_t offset, uint64_t length) const;

	std::string to_string() const;

	private:
	void write_chain(std::shared_ptr<io::writer> &writer);

	item_definition m_item_definition;
	mutable std::variant<reader_kind, sequential_reader_kind, slice_kind, chain_kind, fetch_slice_kind> m_kind;
};
} // namespace archive_diff::diffs::core

template <>
struct fmt::formatter<archive_diff::diffs::core::prepared_item>
{
	constexpr auto parse(fmt::format_parse_context &ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

	auto format(const archive_diff::diffs::core::prepared_item &prepared_item, fmt::format_context &ctx) const
		-> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", prepared_item.to_string());
	}
};