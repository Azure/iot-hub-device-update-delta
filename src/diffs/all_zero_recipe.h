/**
 * @file all_zero_recipe.h
 *
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"
#include "recipe.h"

namespace diffs
{
class zeroes_reader : public io_utility::reader
{
	public:
	zeroes_reader(uint64_t length) : m_length(length) {}
	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer) override;
	virtual uint64_t size() override { return m_length; }
	virtual io_utility::reader::read_style get_read_style() const override
	{
		return io_utility::reader::read_style::random_access;
	}

	private:
	uint64_t m_length{};
};

class all_zero_recipe : public recipe
{
	public:
	all_zero_recipe(const blob_definition &blobdef) : recipe(recipe_type::all_zero, blobdef) {}
	virtual void apply(apply_context &context) const override;
	virtual std::unique_ptr<io_utility::reader> make_reader(apply_context &context) const override;
};
} // namespace diffs
