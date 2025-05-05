/**
 * @file bsdiff_compressor.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <io/reader.h>
#include <io/sequential/writer.h>

namespace archive_diff::io::compressed
{
// bsdiff doesn't easily fit the model of a writer. It takes in two
// input streams and an ouput stream.
// This doesn't fit model of a reader or writer, but that's ok, we can
// simply expose the compression via a custom object, since it's not
// needed for a recipe.
//
class bsdiff_compressor
{
	public:
	static void delta_compress(
		io::reader &old_reader, io::reader &new_reader, std::shared_ptr<io::writer> &diff_writer);
};
}; // namespace archive_diff::io::compressed
