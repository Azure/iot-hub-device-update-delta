/**
 * @file serializer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include <diffs/core/archive.h>
#include <io/sequential/writer.h>

namespace archive_diff::diffs::serialization::standard
{
class serializer
{
	public:
	serializer(std::shared_ptr<diffs::core::archive> &archive) : m_archive(archive) {}

	void write(io::sequential::writer &writer);

	private:
	void write_header(io::sequential::writer &writer);
	void write_suported_recipe_types(io::sequential::writer &writer);
	void write_recipes(io::sequential::writer &writer);
	void write_recipe_set(
		io::sequential::writer &writer, const core::item_definition &result, const core::recipe_set &recipes);
	void write_recipe(io::sequential::writer &writer, const core::recipe &recipe);

	void write_inline_assets(io::sequential::writer &writer);
	void write_remainder(io::sequential::writer &writer);

	void write_nested_archives(io::sequential::writer &writer);

	std::shared_ptr<diffs::core::archive> m_archive;
};
} // namespace archive_diff::diffs::serialization::standard