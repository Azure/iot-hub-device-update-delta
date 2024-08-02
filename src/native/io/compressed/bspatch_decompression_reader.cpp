/**
 * @file bspatch_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "bspatch_decompression_reader.h"

#include "reader_based_bsdiff_stream.h"
#include "sequential_writer_based_bsdiff_stream.h"

namespace archive_diff::io::compressed
{
void bspatch_decompression_reader::call_bspatch(
	io::reader *old_reader, io::reader *diff_reader, io::sequential::writer *new_writer, [[maybe_unused]] void *object)
{
	bsdiff_ctx ctx{0};

	int ret;

	reader_based_bsdiff_stream old_stream(*old_reader);
	reader_based_bsdiff_stream diff_stream(*diff_reader);
	sequential_writer_based_bsdiff_stream new_stream(new_writer);

	ret = bspatch(&ctx, &old_stream, &diff_stream, &new_stream);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bspatch failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}
}
} // namespace archive_diff::io::compressed