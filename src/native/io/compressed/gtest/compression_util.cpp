/**
 * @file compression_util.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "compression_util.h"

#include <io/file/io_device.h>

#include <io/compressed/zstd_decompression_writer.h>

void zstd_uncompress_file(fs::path source, fs::path target)
{
	auto source_reader = archive_diff::io::file::io_device::make_reader(source.string());

	// archive_diff::io::compressed::zstd_decompression_writer writer();
}