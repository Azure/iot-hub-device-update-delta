/**
 * @file bspatch_decompression_reader.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <bsdiff.h>

#include "bspatch_decompression_reader.h"

#include "bsdiff_stream_wrappers.h"

#include "adu_log.h"

namespace archive_diff::io::compressed
{
bspatch_decompression_reader::bspatch_decompression_reader(
	io::reader &diff_reader, uint64_t uncompressed_size, io::reader &dictionary_reader) :
	m_diff_reader(diff_reader), m_dictionary_reader(dictionary_reader),
	m_channel(std::make_shared<writer_to_reader_channel>(uncompressed_size)), m_channel_as_writer(m_channel)
{
	m_channel_thread =
		std::thread(apply_patch_worker, &m_dictionary_reader, &m_channel_as_writer, &m_diff_reader, this);
}

bspatch_decompression_reader::~bspatch_decompression_reader()
{
	m_channel->cancel();
	m_channel_thread.join();
}

void bspatch_decompression_reader::apply_patch_worker(
	io::reader *old_reader,
	std::shared_ptr<io::sequential::writer> *new_writer,
	io::reader *diff_reader,
	[[maybe_unused]] bspatch_decompression_reader *self)
{
	try
	{
		apply_patch(old_reader, new_writer, diff_reader);
	}
	catch (const std::exception &e)
	{
		auto msg = e.what();
		if (msg == nullptr)
		{
			msg = "Unknown error";
		}
		ADU_LOG("bspatch_decompression_reader::apply_patch_worker(): std::exception: {}", msg);
	}
	catch (const errors::user_exception &e)
	{
		ADU_LOG(
			"bspatch_decompression_reader::apply_patch_worker(): user_exception ({}): {}",
			static_cast<uint32_t>(e.get_error()),
			e.get_message());
	}
}

void bspatch_decompression_reader::apply_patch(
	io::reader *old_reader, std::shared_ptr<io::sequential::writer> *new_writer, io::reader *diff_reader)
{
	bsdiff_ctx ctx{0};

	auto_bsdiff_stream old_stream(create_reader_based_bsdiff_stream(*old_reader));
	auto_bsdiff_stream new_stream  = create_sequential_writer_based_bsdiff_stream(*new_writer);
	auto_bsdiff_stream diff_stream = create_reader_based_bsdiff_stream(*diff_reader);

	auto_bsdiff_patch_packer diff_packer;
	int ret = bsdiff_open_bz2_patch_packer(BSDIFF_MODE_READ, &diff_stream, &diff_packer);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bsdiff_open_bz2_patch_packer failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}

	ret = bspatch(&ctx, &old_stream, &new_stream, &diff_packer);
	if (ret != BSDIFF_SUCCESS)
	{
		std::string msg = "bspatch failed. ret: " + std::to_string(ret);
		throw errors::user_exception(errors::error_code::diff_bspatch_failure, msg);
	}
}
} // namespace archive_diff::io::compressed