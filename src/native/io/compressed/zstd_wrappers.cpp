/**
 * @file zstd_wrappers.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "zstd_wrappers.h"

namespace archive_diff::io::compressed
{
int windowLog_from_target_size(uintmax_t target_size)
{
	int windowLog = 0;
	while (target_size)
	{
		target_size >>= 1;
		windowLog++;
	}
	return windowLog;
}
} // namespace archive_diff::io::compressed