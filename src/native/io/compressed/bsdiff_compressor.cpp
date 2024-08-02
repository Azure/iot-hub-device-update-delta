/**
 * @file bsdiff_compressor.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "bsdiff_compressor.h"

#include <bsdiff.h>

#include "reader_based_bsdiff_stream.h"
#include "writer_based_bsdiff_stream.h"

namespace archive_diff::io::compressed
{
void bsdiff_compressor::delta_compress(reader &old_reader, reader &new_reader, writer *writer)
{
	bsdiff_ctx ctx{0};

	int ret;

	reader_based_bsdiff_stream old_stream(old_reader);
	reader_based_bsdiff_stream new_stream(new_reader);
	writer_based_bsdiff_stream diff_stream(writer);

	ret = bsdiff(&ctx, &old_stream, &new_stream, &diff_stream);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bsdiff failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}
}
} // namespace archive_diff::io::compressed