/**
 * @file reader_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"

namespace archive_diff::io::sequential
{
class reader_factory
{
	public:
	virtual ~reader_factory() = default;

	virtual std::unique_ptr<reader> make_sequential_reader() = 0;
};
} // namespace archive_diff::io::sequential