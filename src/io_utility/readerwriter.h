/**
 * @file readerwriter.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "reader.h"
#include "writer.h"

namespace io_utility
{
class readerwriter : public reader, public writer
{
	public:
	virtual ~readerwriter() = default;
};

using unique_readerwriter = std::unique_ptr<readerwriter>;
} // namespace io_utility