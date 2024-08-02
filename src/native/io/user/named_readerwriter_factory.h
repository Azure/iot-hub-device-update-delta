/**
 * @file named_readerwriter_factory.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include "readerwriter.h"

namespace archive_diff::io::user
{
class named_readerwriter_factory
{
	public:
	virtual std::unique_ptr<io::readerwriter> create(const char *name)        = 0;
	virtual std::shared_ptr<io::readerwriter> create_shared(const char *name) = 0;
};
} // namespace archive_diff::io::user