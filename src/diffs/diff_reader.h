/**
 * @file diff_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"

namespace diffs
{
class diff_reader : public io_utility::reader
{
	public:
	diff_reader(
		apply_context &context, io_utility::unique_reader &&source_reader, io_utility::unique_reader &&delta_reader) :
		m_delta_reader(std::move(delta_reader)),
		m_diff(m_delta_reader.get()), m_context(apply_context::nested_context(
										  &context,
										  std::move(source_reader),
										  m_diff.make_inline_assets_reader(m_delta_reader.get()),
										  m_diff.make_remainder_reader(m_delta_reader.get()),
										  m_diff.get_target_size())),
		m_diff_reader(m_diff.make_reader(m_context))
	{}

	virtual size_t read_some(uint64_t offset, gsl::span<char> buffer) override
	{
		return m_diff_reader->read_some(offset, buffer);
	}

	virtual read_style get_read_style() const override { return m_diff_reader->get_read_style(); }

	virtual uint64_t size() override { return m_diff_reader->size(); }

	private:
	io_utility::unique_reader m_delta_reader;

	diffs::diff m_diff;

	diffs::apply_context m_context;

	io_utility::unique_reader m_diff_reader;
};
} // namespace diffs