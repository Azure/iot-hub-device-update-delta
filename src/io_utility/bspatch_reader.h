/**
 * @file bspatch_reader.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <vector>
#include <map>
#include <thread>

#include <string.h>

#include "child_reader.h"
#include "wrapped_reader_sequential_reader.h"
#include "producer_consumer_reader_writer.h"

#include "bsdiff_utility.h"

#include "bsdiff.h"

namespace io_utility
{
class bspatch_decompression_reader : public sequential_reader
{
	public:
	bspatch_decompression_reader(
		io_utility::unique_reader &&source, io_utility::unique_reader &&delta, uint64_t uncompressed_size) :
		m_source_reader(std::move(source)),
		m_delta_reader(std::move(delta)), m_uncompressed_size(uncompressed_size)
	{
		m_target = std::make_unique<producer_consumer_reader_writer>(uncompressed_size);

		m_sink_thread = std::thread(call_bspatch, m_source_reader.get(), m_delta_reader.get(), m_target.get(), this);
	}

	~bspatch_decompression_reader()
	{
		m_target->notify_writer();
		m_sink_thread.join();
	}

	virtual size_t raw_read_some(gsl::span<char> buffer) override { return m_target->read_some(buffer); }

	virtual read_style get_read_style() const override { return read_style::sequential_only; }

	virtual uint64_t size() override { return m_target->size(); }

	private:
	static void call_bspatch(reader *source, reader *delta, producer_consumer_reader_writer *sink, void *object)
	{
		bsdiff_ctx ctx{0};

		int ret;

		auto source_stream = std::make_unique<io_utility::bsdiff_utility::reader_based_bsdiff_stream>(source);
		auto delta_stream  = std::make_unique<io_utility::bsdiff_utility::reader_based_bsdiff_stream>(delta);
		auto target_stream = std::make_unique<io_utility::bsdiff_utility::writer_based_bsdiff_stream>(sink);

		ret = bspatch(&ctx, source_stream.get(), delta_stream.get(), target_stream.get());
		if (ret != BSDIFF_SUCCESS)
		{
			std::string msg = "bspatch failed. ret: " + std::to_string(ret);
			throw error_utility::user_exception(error_utility::error_code::diff_bspatch_failure, msg);
		}
	}

	uint64_t m_uncompressed_size{};
	io_utility::unique_reader m_source_reader;
	io_utility::unique_reader m_delta_reader;

	std::unique_ptr<io_utility::producer_consumer_reader_writer> m_target;

	bool m_cancel{false};
	std::thread m_sink_thread;
};
} // namespace io_utility