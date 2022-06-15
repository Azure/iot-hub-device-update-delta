/**
 * @file bsdiff_utility.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include "reader.h"
#include "writer.h"
#include "producer_consumer_reader_writer.h"

#include "bsdiff.h"

namespace io_utility
{
class bsdiff_utility
{
	public:
	class writer_based_bsdiff_stream : public bsdiff_stream
	{
		public:
		writer_based_bsdiff_stream()          = delete;
		virtual ~writer_based_bsdiff_stream() = default;

		writer_based_bsdiff_stream(io_utility::writer *writer) : m_writer(writer)
		{
			this->seek  = nullptr;
			this->tell  = tell_impl;
			this->read  = nullptr;
			this->write = write_impl;
			this->flush = flush_impl;
			this->close = close_impl;

			state = this;
		}

		static int tell_impl(void *state, int64_t *position)
		{
			auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
			*position    = thisPtr->m_offset;

			return BSDIFF_SUCCESS;
		}

		static int write_impl(void *state, const void *buffer, size_t size)
		{
			auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
			auto writer  = thisPtr->m_writer;

			try
			{
				writer->write(thisPtr->m_offset, std::string_view{reinterpret_cast<const char *>(buffer), size});
			}
			catch (producer_consumer_reader_writer::writer_cancelled_exception &)
			{
				return -1;
			}
			thisPtr->m_offset += size;

			return BSDIFF_SUCCESS;
		}

		static int flush_impl(void *state)
		{
			auto thisPtr = reinterpret_cast<writer_based_bsdiff_stream *>(state);
			auto writer  = thisPtr->m_writer;
			writer->flush();

			return BSDIFF_SUCCESS;
		}

		static void close_impl(void *state) {}

		private:
		io_utility::writer *m_writer;
		int64_t m_offset{};
	};

	class reader_based_bsdiff_stream : public bsdiff_stream
	{
		public:
		reader_based_bsdiff_stream()          = delete;
		virtual ~reader_based_bsdiff_stream() = default;

		reader_based_bsdiff_stream(io_utility::reader *reader) : m_reader(reader)
		{
			this->seek  = seek_impl;
			this->tell  = tell_impl;
			this->read  = read_impl;
			this->write = nullptr; // we are readonly so we shouldn't implement write/flush
			this->flush = nullptr;
			this->close = close_impl;

			state = this;
		}

		static int seek_impl(void *state, int64_t offset, int origin)
		{
			auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);

			switch (origin)
			{
			case SEEK_SET:
				thisPtr->m_offset = offset;
				break;
			case SEEK_CUR:
				thisPtr->m_offset += offset;
				break;
			case SEEK_END:
				auto reader       = thisPtr->m_reader;
				thisPtr->m_offset = reader->size() + offset;
				break;
			}

			return BSDIFF_SUCCESS;
		}

		static int tell_impl(void *state, int64_t *position)
		{
			auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);
			*position    = thisPtr->m_offset;

			return BSDIFF_SUCCESS;
		}

		static int read_impl(void *state, void *buffer, size_t size, size_t *readed)
		{
			auto thisPtr = reinterpret_cast<reader_based_bsdiff_stream *>(state);
			auto reader  = thisPtr->m_reader;
			auto offset  = thisPtr->m_offset;

			*readed = reader->read_some(offset, gsl::span<char>(reinterpret_cast<char *>(buffer), size));

			return BSDIFF_SUCCESS;
		}

		static int write_impl(void *state, const void *buffer, size_t size) { return BSDIFF_SUCCESS; }

		static int flush_impl(void *state) { return BSDIFF_SUCCESS; }

		static void close_impl(void *state) {}

		private:
		io_utility::reader *m_reader;
		int64_t m_offset{};
	};
};
} // namespace io_utility
