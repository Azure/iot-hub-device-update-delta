/**
 * @file dump_diff.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "dump_diff.h"

#include "diff.h"
#include "archive_item.h"
#include "recipe_helpers.h"

#include "child_reader.h"
#include "binary_file_reader.h"

void dump_diff(const diffs::diff &diff, const std::string &indent, io_utility::reader &reader, std::ostream &ostream);

void diffs::dump_diff(const std::string &diff_path, std::ostream &ostream)
{
	io_utility::binary_file_reader reader(diff_path);
	diffs::diff diff(&reader);

	::dump_diff(diff, "  ", reader, ostream);
}

void dump_raw_remainder_data(io_utility::reader &reader, std::ostream &ostream);
void dump_remainder_data(io_utility::sequential_reader &reader, std::ostream &ostream);
void dump_archive_item(
	const diffs::archive_item &item,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent,
	std::ostream &ostream);

void dump_diff(const diffs::diff &diff, const std::string &indent, io_utility::reader &reader, std::ostream &ostream)
{
	ostream << indent << "Diff file size            : " << diff.get_diff_size() << std::endl;
	ostream << indent << "Target file size          : " << diff.get_target_size() << std::endl;
	ostream << indent << "Target file hash          : " << diff.get_target_hash().get_string() << std::endl;

	auto source_size = diff.get_source_size();

	if (source_size != 0)
	{
		ostream << indent << "Source file size          : " << diff.get_source_size() << std::endl;
		ostream << indent << "Source file hash          : " << diff.get_source_hash().get_string() << std::endl;
	}

	auto inline_assets_offset = diff.get_inline_assets_offset();
	auto inline_assets_size   = diff.get_inline_assets_size();
	ostream << indent << "Inline Asset offset       : " << inline_assets_offset << std::endl;
	ostream << indent << "Inline Asset size         : " << inline_assets_size << std::endl;
	ostream << indent << "Remainder offset          : " << diff.get_remainder_offset() << std::endl;
	ostream << indent << "Remainder size            : " << diff.get_remainder_size() << std::endl;
	ostream << indent << "Remainder size(compressed): " << diff.get_remainder_uncompressed_size() << std::endl;

	io_utility::child_reader raw_remainder_data(&reader, inline_assets_offset, inline_assets_size);
	ostream << indent;
	dump_raw_remainder_data(raw_remainder_data, ostream);

	auto remainder_reader = diff.make_remainder_reader(&reader);
	ostream << indent;
	dump_remainder_data(*remainder_reader.get(), ostream);

	const auto &chunks = diff.get_chunks();
	ostream << "Diff Chunks (Count: " << chunks.size() << ")" << std::endl;

	auto indent_next = indent + "  ";

	for (size_t i = 0; i < chunks.size(); i++)
	{
		dump_archive_item(chunks[i], &diff, reader, indent_next, ostream);
	}
}

void hash_and_dump(io_utility::reader &reader, std::ostream &ostream)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);

	auto size      = reader.size();
	auto remaining = size;

	const size_t max_read_buffer_size = static_cast<size_t>(32) * 1024;
	auto buffer_size                  = std::min<size_t>(max_read_buffer_size, size);

	std::vector<char> read_buffer;
	read_buffer.reserve(buffer_size);

	io_utility::wrapped_reader_sequential_reader sequential_reader(&reader);

	while (remaining)
	{
		auto to_read = std::min<size_t>(buffer_size, remaining);
		sequential_reader.read(gsl::span{read_buffer.data(), to_read});

		hasher.hash_data(std::string_view{read_buffer.data(), to_read});

		remaining -= to_read;
	}

	auto hash_string = hasher.get_hash_string();
	ostream << "Size: " << size << ", Hash : " << hash_string << std::endl;
}

void hash_and_dump(std::string_view data, std::ostream &ostream)
{
	hash_utility::hasher hasher(hash_utility::algorithm::SHA256);
	hasher.hash_data(data);
	auto hash_string = hasher.get_hash_string();
	ostream << "Size: " << data.size() << ", Hash : " << hash_string << std::endl;
}

void dump_raw_remainder_data(io_utility::reader &reader, std::ostream &ostream)
{
	ostream << "Remainder Data(compressed)  : ";
	hash_and_dump(reader, ostream);
}

void dump_remainder_data(io_utility::sequential_reader &reader, std::ostream &ostream)
{
	ostream << "Remainder Data(uncompressed): ";
	hash_and_dump(reader, ostream);
}

void dump_recipe(
	const diffs::recipe &recipe,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent,
	std::ostream &ostream);

void dump_archive_item(
	const diffs::archive_item &item,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent,
	std::ostream &ostream)
{
	auto offset = item.get_offset();
	auto length = item.get_length();
	auto &hash  = item.get_hash();

	ostream << indent << "Chunk. Offset: " << offset << " Length: " << length << " Hash: " << hash.get_string()
			<< std::endl;
	std::string indent_next = indent + "  ";

	if (item.has_recipe())
	{
		auto recipe = item.get_recipe();
		dump_recipe(*recipe, diff, reader, indent_next, ostream);
	}
}

void dump_recipe_parameter(
	const diffs::recipe_parameter &param,
	size_t index,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent_next,
	std::ostream &ostream);

void dump_recipe(
	const diffs::recipe &recipe,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent,
	std::ostream &ostream)
{
	const auto &parameters = recipe.get_parameters();

	std::string recipe_type_name = recipe.get_recipe_type_name();

	ostream << indent << "Recipe. Method: " << recipe.get_recipe_type_name() << " ParameterCount: " << parameters.size()
			<< std::endl;

	if (parameters.size() == 0)
	{
		return;
	}

	std::string indent_next = indent + "  ";
	ostream << indent_next << "Parameters: " << std::endl;
	for (size_t i = 0; i < parameters.size(); i++)
	{
		auto &param = parameters[i];
		dump_recipe_parameter(param, i, diff, reader, indent_next, ostream);
	}

	if (recipe_type_name.compare("nested_diff") == 0)
	{
		auto delta_item = parameters[diffs::RECIPE_PARAMETER_DELTA].get_archive_item_value();

		if (!delta_item->has_recipe())
		{
			throw error_utility::user_exception(error_utility::error_code::diff_dump_nested_delta_item_missing_recipe);
		}

		auto delta_recipe = delta_item->get_recipe();

		std::string delta_recipe_type_name = delta_recipe->get_recipe_type_name();

		if (delta_recipe_type_name.compare("inline_asset") != 0)
		{
			throw error_utility::user_exception(
				error_utility::error_code::diff_dump_nested_delta_item_not_inline_asset);
		}

		auto offset = delta_recipe->get_parameters()[diffs::RECIPE_PARAMETER_OFFSET].get_number_value();
		auto length = delta_recipe->get_parameters()[diffs::RECIPE_PARAMETER_LENGTH].get_number_value();

		auto inline_assets_reader = diff->make_inline_assets_reader(&reader);

		io_utility::child_reader nested_diff_reader(inline_assets_reader.get(), offset, length);

		diffs::diff nested_diff(&nested_diff_reader);

		::dump_diff(nested_diff, indent_next, nested_diff_reader, ostream);
	}
}

void dump_recipe_parameter(
	const diffs::recipe_parameter &param,
	size_t index,
	const diffs::diff *diff,
	io_utility::reader &reader,
	const std::string &indent,
	std::ostream &ostream)
{
	auto type = param.get_type();

	ostream << indent << "[" << index << "] ";

	switch (type)
	{
	case diffs::recipe_parameter_type::number:
		ostream << "Type: number Value: " << param.get_number_value() << std::endl;
		break;
	case diffs::recipe_parameter_type::archive_item: {
		ostream << "Type: archive_item" << std::endl;
		std::string indent_next = indent + "  ";
		dump_archive_item(*param.get_archive_item_value(), diff, reader, indent_next, ostream);
		break;
	}
	default:
		throw error_utility::user_exception(error_utility::error_code::diff_dump_parameter_invalid_type);
	}
}