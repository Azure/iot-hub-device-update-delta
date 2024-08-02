/**
 * @file basic_writer_wrapper.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <memory>

#include "writer_impl.h"

namespace archive_diff::io::sequential
{
template <typename WriterT>
class basic_writer_wrapper_template : public writer_impl
{
	public:
	basic_writer_wrapper_template(std::shared_ptr<WriterT> &writer) : m_writer(writer) {}

	virtual void flush() { m_writer->flush(); }

	static std::shared_ptr<archive_diff::io::sequential::writer> make_shared(std::shared_ptr<WriterT> &writer)
	{
		return std::make_shared<basic_writer_wrapper_template>(writer);
	}

	static std::shared_ptr<archive_diff::io::sequential::writer> make_unique(std::shared_ptr<WriterT> &writer)
	{
		return std::make_unique<basic_writer_wrapper_template>(writer);
	}

	protected:
	virtual void write_impl(std::string_view buffer) { m_writer->write(m_offset, buffer); }

	private:
	std::shared_ptr<WriterT> m_writer{};
};
using basic_writer_wrapper = basic_writer_wrapper_template<io::writer>;
} // namespace archive_diff::io::sequential