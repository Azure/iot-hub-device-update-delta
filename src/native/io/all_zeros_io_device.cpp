/**
 * @file all_zeros_io_device.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "all_zeros_io_device.h"

#include <cstring>

namespace archive_diff::io
{
size_t all_zeros_io_device::read_some(uint64_t offset, std::span<char> buffer)
{
	auto available = std::max((uint64_t)0, m_length - offset);
	auto to_read   = std::min<size_t>(available, buffer.size());

	std::memset(buffer.data(), 0, to_read);
	return to_read;
}
} // namespace archive_diff::io