/**
 * @file prepared_item.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "prepared_item.h"

#include <io/sequential/basic_reader_wrapper.h>
#include <io/sequential/chain_reader.h>
#include <io/hashed/hashed_sequential_writer.h>

#include <io/file/temp_file.h>

#include <language_support/overload_pattern.h>

#include "kitchen.h"

namespace archive_diff::diffs::core
{
bool prepared_item::can_make_reader() const
{
	return std::visit(
		overload{
			[](reader_kind &) { return true; },
			[](sequential_reader_kind &) { return false; },
			[](slice_kind &kind) { return kind.m_item->can_make_reader(); },
			[](chain_kind &kind)
			{
				for (auto &item : kind.m_items)
				{
					if (!item->can_make_reader())
					{
						return false;
					}
				}

				return true;
			},
			[](fetch_slice_kind &) { return true; },
		},
		m_kind);
}

io::reader prepared_item::make_reader()
{
	return std::visit(
		overload{
			[](reader_kind &kind) { return kind.m_factory->make_reader(); },
			[](sequential_reader_kind &kind)
			{
				auto temp_file = std::make_shared<io::file::temp_file>();

				auto seq_reader = kind.m_factory->make_sequential_reader();

				io::shared_writer writer = std::make_shared<io::file::temp_file_writer>(temp_file);
				io::sequential::basic_writer_wrapper wrapper(writer);

				wrapper.write(*seq_reader);

				return io::file::temp_file_io_device::make_reader(temp_file);
			},
			[](slice_kind &kind)
			{
				auto reader = kind.m_item->make_reader();
				return reader.slice(kind.m_offset, kind.m_length);
			},
			[](chain_kind &kind)
			{
				std::vector<io::reader> readers;
				for (auto &item : kind.m_items)
				{
					readers.push_back(item->make_reader());
				}
				return io::reader::chain(readers);
			},
			[&](fetch_slice_kind &kind)
			{
				// printf("Fetching slice from kitchen...\n");
				std::shared_ptr<prepared_item> result;

				// We're going to try to mutate into the reader version
		        // by fetching from the kitchen.
		        // We may wait here while more works is done as a result.
				if (auto kitchen = kind.kitchen.lock())
				{
					result = kitchen->fetch_slice(m_item_definition);
				}
				else
				{
					// kitchen is gone!
					ADU_LOG("Kitchen missing when trying to prepare reader.");
					throw errors::user_exception(errors::error_code::diffs_prepared_item_kitchen_gone);
				}

				if (!std::holds_alternative<reader_kind>(result->m_kind))
				{
					throw errors::user_exception(
						errors::error_code::diffs_prepared_item_wrong_kind, "Fetched item not reader kind");
				}

				m_kind = result->m_kind;

				auto &factory = std::get<reader_kind>(m_kind).m_factory;
				return factory->make_reader();
			},
		},
		m_kind);
}

bool prepared_item::can_slice([[maybe_unused]] uint64_t offset, [[maybe_unused]] uint64_t length) const
{
	return can_make_reader();
}

std::unique_ptr<io::sequential::reader> prepared_item::make_sequential_reader() const
{
	return std::visit(
		overload{
			[](reader_kind &kind)
			{
				auto reader = kind.m_factory->make_reader();
				std::unique_ptr<io::sequential::reader> result =
					std::make_unique<io::sequential::basic_reader_wrapper>(reader);
				return result;
			},
			[](sequential_reader_kind &kind) { return kind.m_factory->make_sequential_reader(); },
			[](slice_kind &kind)
			{
				auto reader = kind.m_item->make_reader();
				auto slice  = reader.slice(kind.m_offset, kind.m_length);
				std::unique_ptr<io::sequential::reader> result =
					std::make_unique<io::sequential::basic_reader_wrapper>(slice);
				return result;
			},
			[](chain_kind &kind)
			{
				std::vector<std::unique_ptr<io::sequential::reader>> readers;
				for (auto &item : kind.m_items)
				{
					readers.emplace_back(item->make_sequential_reader());
				}
				std::unique_ptr<io::sequential::reader> result =
					std::make_unique<io::sequential::chain_reader>(std::move(readers));
				return result;
			},
			[&](fetch_slice_kind &kind)
			{
				// printf("Fetching slice from kitchen...\n");
				std::shared_ptr<prepared_item> fetched_slice;

				// We're going to try to mutate into the reader version
		        // by fetching from the kitchen.
		        // We may wait here while more works is done as a result.
				if (auto kitchen = kind.kitchen.lock())
				{
					fetched_slice = kitchen->fetch_slice(m_item_definition);
				}
				else
				{
					// kitchen is gone!
					ADU_LOG("Kitchen missing when trying to prepare reader.");
					throw errors::user_exception(errors::error_code::diffs_prepared_item_kitchen_gone);
				}

				if (!std::holds_alternative<reader_kind>(fetched_slice->m_kind))
				{
					throw errors::user_exception(
						errors::error_code::diffs_prepared_item_wrong_kind, "Fetched item not reader kind");
				}

				m_kind = fetched_slice->m_kind;

				auto &factory = std::get<reader_kind>(m_kind).m_factory;
				auto reader   = factory->make_reader();
				std::unique_ptr<io::sequential::reader> result =
					std::make_unique<io::sequential::basic_reader_wrapper>(reader);
				return result;
			},
		},
		m_kind);
}

void prepared_item::write_chain([[maybe_unused]] std::shared_ptr<io::writer> &writer)
{
	auto chain = std::get<chain_kind>(m_kind);

	io::sequential::basic_writer_wrapper seq_writer(writer);

	for (auto &item : chain.m_items)
	{
		auto reader = item->make_sequential_reader();
		seq_writer.write(*reader);
	}
}

void prepared_item::write([[maybe_unused]] std::shared_ptr<io::writer> &writer)
{
	auto reader = make_sequential_reader();
#if 0
	auto hasher = std::make_shared<hashing::hasher>(hashing::algorithm::sha256);
	io::hashed::hashed_sequential_writer hashed_writer(writer, hasher);

	hashed_writer.write(*reader);
#else
	io::sequential::basic_writer_wrapper seq_writer(writer);
	seq_writer.write(*reader);
#endif
}

std::string prepared_item::to_string() const
{
	std::string str = fmt::format("{{result: {}, ", m_item_definition);

	std::visit(
		overload{
			[&](reader_kind &reader)
			{
				str += "reader";

				if (reader.m_ingredients.empty())
				{
					return;
				}

				str += ", {ingredients: {";
				auto item = reader.m_ingredients.cbegin();
				while (item != reader.m_ingredients.cend())
				{
					str += (*item)->to_string();
					item++;
					if (item != reader.m_ingredients.cend())
					{
						str += ", ";
					}
				}
				str += "}";
			},
			[&](sequential_reader_kind &reader)
			{
				str += "sequential_reader";

				if (reader.m_ingredients.empty())
				{
					return;
				}

				str += ", {ingredients: {";
				auto item = reader.m_ingredients.cbegin();
				while (item != reader.m_ingredients.cend())
				{
					str += (*item)->to_string();
					item++;
					if (item != reader.m_ingredients.cend())
					{
						str += ", ";
					}
				}
				str += "}";
			},
			[&](slice_kind &slice)
			{
				str +=
					fmt::format("slice: {{offset: {}, length: {}, {}}}", slice.m_offset, slice.m_length, *slice.m_item);
			},
			[&](chain_kind &chain)
			{
				str += "chain: {";
				auto item = chain.m_items.cbegin();
				while (item != chain.m_items.cend())
				{
					str += (*item)->to_string();
					item++;
					if (item != chain.m_items.cend())
					{
						str += ", ";
					}
				}
				str += "}";
			},
			[&]([[maybe_unused]] fetch_slice_kind &fetch) { str += "kitchen_fetch_slice"; },
			[](auto)
			{
				throw errors::user_exception(
					errors::error_code::diffs_prepared_item_unkown, "Unknown kind when trying to convert to string.");
			},
		},
		m_kind);

	str += "}";
	return str;
}
} // namespace archive_diff::diffs::core