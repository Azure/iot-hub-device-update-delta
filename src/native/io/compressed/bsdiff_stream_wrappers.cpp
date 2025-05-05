#include "bsdiff_stream_wrappers.h"

namespace archive_diff::io::compressed
{
struct bsdiff_reader_stream_context
{
	bsdiff_reader_stream_context(io::reader &reader) : reader(reader) {}

	io::reader reader;
	uint64_t offset{0};
};

struct bsdiff_writer_stream_context
{
	bsdiff_writer_stream_context(std::shared_ptr<io::writer> &writer) : writer(writer) {}

	std::shared_ptr<io::writer> writer;
	uint64_t offset{0};
};

struct bsdiff_sequential_writer_stream_context
{
	bsdiff_sequential_writer_stream_context(std::shared_ptr<io::sequential::writer> &writer) :
		writer(writer.get()), writer_storage(writer)
	{}
	bsdiff_sequential_writer_stream_context(io::sequential::writer *writer) : writer(writer) {}

	io::sequential::writer *writer{nullptr};
	std::shared_ptr<io::sequential::writer> writer_storage;
	int64_t expected_seek{0};
};

void reader_stream_close(void *state)
{
	auto context = static_cast<bsdiff_reader_stream_context *>(state);
	if (context)
	{
		delete context;
	}
}

int reader_stream_get_mode(void *) { return BSDIFF_MODE_READ; }

int reader_stream_seek(void *state, int64_t offset, int origin)
{
	auto context = static_cast<bsdiff_reader_stream_context *>(state);

	if (!context)
	{
		return BSDIFF_ERROR;
	}

	switch (origin)
	{
	case SEEK_SET:
		context->offset = offset;
		break;
	case SEEK_CUR:
		context->offset += offset;
		break;
	case SEEK_END:
		context->offset = context->reader.size() + offset;
		break;
	default:
		return BSDIFF_ERROR;
	}
	return BSDIFF_SUCCESS;
}

int reader_stream_tell(void *state, int64_t *position)
{
	auto context = static_cast<bsdiff_reader_stream_context *>(state);

	if (!context)
	{
		return BSDIFF_ERROR;
	}

	*position = context->offset;
	return BSDIFF_SUCCESS;
}

int reader_stream_read(void *state, void *buffer, size_t size, size_t *readed)
{
	auto context = static_cast<bsdiff_reader_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	*readed = context->reader.read_some(context->offset, std::span<char>(reinterpret_cast<char *>(buffer), size));

	return (*readed < size) ? BSDIFF_END_OF_FILE : BSDIFF_SUCCESS;
}

bsdiff_stream create_reader_based_bsdiff_stream(io::reader &reader)
{
	auto state = std::make_unique<bsdiff_reader_stream_context>(reader);

	bsdiff_stream stream;

	stream.close    = reader_stream_close;
	stream.get_mode = reader_stream_get_mode;
	stream.seek     = reader_stream_seek;
	stream.tell     = reader_stream_tell;
	/* read mode only */
	stream.read = reader_stream_read;

	/* write mode only */
	stream.flush = nullptr;
	stream.write = nullptr;

	/* optional */
	stream.get_buffer = nullptr;

	stream.state = state.release();

	return stream;
}

void writer_stream_close(void *state)
{
	auto context = static_cast<bsdiff_writer_stream_context *>(state);
	if (context)
	{
		context->writer.reset();
		delete context;
	}
}

int writer_stream_get_mode(void *) { return BSDIFF_MODE_WRITE; }

int writer_stream_seek(void *state, int64_t offset, int origin)
{
	auto context = static_cast<bsdiff_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	switch (origin)
	{
	case SEEK_SET:
		context->offset = offset;
		break;
	case SEEK_CUR:
		context->offset += offset;
		break;
	case SEEK_END:
		return BSDIFF_ERROR;
		break;
	default:
		return BSDIFF_ERROR;
	}

	return BSDIFF_SUCCESS;
}

int writer_stream_tell(void *state, int64_t *position)
{
	auto context = static_cast<bsdiff_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	*position = context->offset;
	return BSDIFF_SUCCESS;
}

int writer_stream_flush(void *state)
{
	auto context = static_cast<bsdiff_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	context->writer->flush();
	return BSDIFF_SUCCESS;
}

int writer_stream_write(void *state, const void *buffer, size_t size)
{
	auto context = static_cast<bsdiff_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	context->writer->write(context->offset, std::string_view(reinterpret_cast<const char *>(buffer), size));
	context->offset += size;
	return BSDIFF_SUCCESS;
}

bsdiff_stream create_writer_based_bsdiff_stream(
	std::shared_ptr<io::writer> &writer)
{
	auto state = std::make_unique<bsdiff_writer_stream_context>(writer);

	bsdiff_stream stream;

	stream.close    = writer_stream_close;
	stream.get_mode = writer_stream_get_mode;
	stream.seek     = writer_stream_seek;
	stream.tell     = writer_stream_tell;
	/* read mode only */
	stream.read = nullptr;

	/* write mode only */
	stream.flush = writer_stream_flush;
	stream.write = writer_stream_write;

	/* optional */
	stream.get_buffer = nullptr;

	stream.state = state.release();

	return stream;
}

void sequential_writer_stream_close(void *state)
{
	auto context = static_cast<bsdiff_sequential_writer_stream_context *>(state);
	if (context)
	{
		context->writer_storage.reset();
		delete context;
	}
}

int sequential_writer_stream_get_mode(void *) { return BSDIFF_MODE_WRITE; }

int sequential_writer_stream_seek(void *state, int64_t offset, int origin)
{
	auto context = static_cast<bsdiff_sequential_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	switch (origin)
	{
	case SEEK_SET:
		return BSDIFF_ERROR;
		break;
	case SEEK_CUR:
		if (offset != context->expected_seek)
		{
			return BSDIFF_ERROR;
		}
		break;
	case SEEK_END:
		return BSDIFF_ERROR;
		break;
	default:
		return BSDIFF_ERROR;
	}

	return BSDIFF_SUCCESS;
}

int sequential_writer_stream_tell(void *state, int64_t *position)
{
	auto context = static_cast<bsdiff_sequential_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	*position = context->writer->tellp();
	return BSDIFF_SUCCESS;
}

int sequential_writer_stream_flush(void *state)
{
	auto context = static_cast<bsdiff_sequential_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	context->writer->flush();
	return BSDIFF_SUCCESS;
}

int sequential_writer_stream_write(void *state, const void *buffer, size_t size)
{
	auto context = static_cast<bsdiff_sequential_writer_stream_context *>(state);
	if (!context)
	{
		return BSDIFF_ERROR;
	}

	context->writer->write(std::string_view(reinterpret_cast<const char *>(buffer), size));
	context->expected_seek += size;

	return BSDIFF_SUCCESS;
}

bsdiff_stream create_sequential_writer_based_bsdiff_stream(
	std::shared_ptr<io::sequential::writer> &writer)
{
	auto state = std::make_unique<bsdiff_sequential_writer_stream_context>(writer);

	bsdiff_stream stream;

	stream.close    = sequential_writer_stream_close;
	stream.get_mode = sequential_writer_stream_get_mode;
	stream.seek     = sequential_writer_stream_seek;
	stream.tell     = sequential_writer_stream_tell;
	/* read mode only */
	stream.read = nullptr;

	/* write mode only */
	stream.flush = sequential_writer_stream_flush;
	stream.write = sequential_writer_stream_write;

	/* optional */
	stream.get_buffer = nullptr;

	stream.state = state.release();

	return stream;
}
} // namespace archive_diff::io::compressed