/**
 * @file reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "reader.h"

namespace io_utility
{
class reader_factory
{
	public:
	virtual ~reader_factory() = default;

	virtual std::unique_ptr<reader> create() = 0;
};
} // namespace io_utility