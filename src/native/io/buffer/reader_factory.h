/**
 * @file reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>

#include <io/reader_factory.h>

#include "io_device.h"

namespace archive_diff::io::buffer
{
class reader_factory : public io::reader_factory
{
	public:
	virtual ~reader_factory() = default;

	reader_factory(std::shared_ptr<std::vector<char>> &buffer, io_device::size_kind size_kind) :
		m_buffer(buffer), m_size_kind(size_kind)
	{}

	virtual io::reader make_reader() override { return io_device::make_reader(m_buffer, m_size_kind); }

	private:
	std::shared_ptr<std::vector<char>> m_buffer;
	io_device::size_kind m_size_kind;
};
} // namespace archive_diff::io::buffer
