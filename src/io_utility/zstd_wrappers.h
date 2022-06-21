/**
 * @file zstd_wrappers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "zstd.h"

namespace io_utility
{
struct ZSTD_CStreamDeleter
{
	void operator()(ZSTD_CStream *obj) { ZSTD_freeCStream(obj); }
};

using unique_zstd_cstream = std::unique_ptr<ZSTD_CStream, ZSTD_CStreamDeleter>;

struct ZSTD_DStreamDeleter
{
	void operator()(ZSTD_DStream *obj) { ZSTD_freeDStream(obj); }
};

using unique_zstd_dstream = std::unique_ptr<ZSTD_DStream, ZSTD_DStreamDeleter>;

static int windowLog_from_target_size(uintmax_t target_size)
{
	int windowLog = 0;
	while (target_size)
	{
		target_size >>= 1;
		windowLog++;
	}
	return windowLog;
}

const int c_zstd_window_log_max = 28; /* sets limit to 1 << 28 or around 268 MB */
} // namespace io_utility