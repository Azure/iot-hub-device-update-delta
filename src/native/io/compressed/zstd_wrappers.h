/**
 * @file zstd_wrappers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>

#include "zstd.h"

namespace archive_diff::io::compressed
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

int windowLog_from_target_size(uintmax_t target_size);

const int c_zstd_window_log_max = 28; /* sets limit to 1 << 28 or around 268 MB */
} // namespace archive_diff::io::compressed