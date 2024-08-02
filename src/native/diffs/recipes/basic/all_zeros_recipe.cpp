/**
 * @file all_zeros_recipe.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "all_zeros_recipe.h"

#include <io/all_zeros_io_device.h>

namespace archive_diff::diffs::recipes::basic
{
class zeros_reader_factory : public io::reader_factory
{
	public:
	zeros_reader_factory(uint64_t length) : m_length(length) {}

	virtual io::reader make_reader() override { return io::all_zeros_io_device::make_reader(m_length); }

	private:
	uint64_t m_length{};
};

all_zeros_recipe::all_zeros_recipe(
	const item_definition &result_item_definition,
	const std::vector<uint64_t> &number_ingredients,
	const std::vector<item_definition> &item_ingredients) :
	recipe(result_item_definition, number_ingredients, item_ingredients)
{}

diffs::core::recipe::prepare_result all_zeros_recipe::prepare(
	[[maybe_unused]] kitchen *kitchen, [[maybe_unused]] std::vector<std::shared_ptr<prepared_item>> &items) const
{
	std::shared_ptr<io::reader_factory> factory =
		std::make_shared<zeros_reader_factory>(m_result_item_definition.size());
	return std::make_shared<prepared_item>(m_result_item_definition, diffs::core::prepared_item::reader_kind{factory});
}
} // namespace archive_diff::diffs::recipes::basic
