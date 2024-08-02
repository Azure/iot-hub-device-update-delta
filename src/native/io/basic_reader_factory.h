/**
 * @file basic_reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <memory>
#include <vector>

#include <io/reader_factory.h>

#include "io_device.h"

namespace archive_diff::io
{
class basic_reader_factory : public io::reader_factory
{
	public:
	virtual ~basic_reader_factory() = default;

	basic_reader_factory(reader& reader) :
		m_reader(reader)
	{}

	virtual io::reader make_reader() override { return m_reader; }

	private:
	reader m_reader;
};
} // namespace archive_diff::io::buffer
