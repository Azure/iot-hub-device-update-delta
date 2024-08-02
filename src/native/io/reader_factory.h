/**
 * @file reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "reader.h"

namespace archive_diff::io
{
class reader_factory
{
	public:
	virtual ~reader_factory() = default;

	virtual io::reader make_reader() = 0;
};
}; // namespace archive_diff::io