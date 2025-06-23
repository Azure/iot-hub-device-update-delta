/**
 * @file bsdiff_compressor.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "bsdiff_compressor.h"

#include <bsdiff.h>

#include "bsdiff_stream_wrappers.h"

namespace archive_diff::io::compressed
{
void bsdiff_compressor::delta_compress(
	io::reader &old_reader, io::reader &new_reader, std::shared_ptr<io::writer> &diff_writer)
{
	bsdiff_ctx ctx{0};

	auto_bsdiff_stream old_stream  = create_reader_based_bsdiff_stream(old_reader);
	auto_bsdiff_stream new_stream  = create_reader_based_bsdiff_stream(new_reader);
	auto_bsdiff_stream diff_stream = create_writer_based_bsdiff_stream(diff_writer);

	auto_bsdiff_patch_packer diff_packer;
	int ret = bsdiff_open_bz2_patch_packer(BSDIFF_MODE_WRITE, &diff_stream, &diff_packer);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bsdiff_open_bz2_patch_packer failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}

	ret = bsdiff(&ctx, &old_stream, &new_stream, &diff_packer);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bsdiff failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}
}
} // namespace archive_diff::io::compressed