#include "diff_api.h"
#include "aduapi_type_conversion.h"

#include <errors/error_codes.h>
#include <errors/user_exception.h>
#include <io/compressed/zlib_compression_reader.h>
#include <io/file/io_device.h>
#include <io/sequential/basic_writer_wrapper.h>

using namespace archive_diff;

bool try_recompress(io::reader &uncompressed_reader, io::reader &compressed_reader, int32_t compression_level);

// This API takes in an uncompressed file along with the same file compressed using ZLIB and attempts to determine
// the level of ZLIB compression applied to replicate the compressed file.
// Parameters:
//   uncompressed_file_path: Path to uncompressed file
//   compressed_file_path: Path to compressed file
//   compression_level: output parameter that will contain the compression level we determine
// Return: If the function completes successfully, zero is returned. If there is an error, such as not being able to
// determine any compression type, then the function will return a non-zero value.
extern "C" ADUAPI_LINKAGESPEC uint32_t diff_get_zlib_compression_level(
	const char *uncompressed_file_path, const char *compressed_file_path, int32_t *compression_level)
{
	try
	{
		auto uncompressed_reader = io::file::io_device::make_reader(uncompressed_file_path);
		auto compressed_reader   = io::file::io_device::make_reader(compressed_file_path);

		// Z_BEST_COMPRESSION is 9 and most commonly selected for an archive like this
		// Z_BEST_SPEED is 1 and allows for faster builds so may be selected commonly
		// Z_DEFAULT_COMPRESSION is -1 and is default so is a commonly selected
		// 5 is balanced compression/speed so common choice
		// Other choices are not-common, but possible
		int32_t compression_levels_to_try[] = {
			Z_BEST_COMPRESSION, Z_BEST_SPEED, Z_DEFAULT_COMPRESSION, 5, 2, 3, 4, 6, 7, 8};

		// Try each compression level in order specified to see if we can replicate the compression
		// supplied in the compressed file
		for (auto compression_level_to_try : compression_levels_to_try)
		{
			if (try_recompress(uncompressed_reader, compressed_reader, compression_level_to_try))
			{
				*compression_level = compression_level_to_try;
				return 0;
			}
		}

		ADU_LOG(
			"diff_get_zlib_compression_level(): Could not determine zlib compression level. Uncompressed path: {}, "
			"Uncompressed path: {}",
			uncompressed_file_path,
			compressed_file_path);
		return static_cast<uint32_t>(errors::error_code::api_unknown_zlib_compression);
	}
	catch (std::exception &e)
	{
		auto msg = e.what();
		if (msg)
		{
			ADU_LOG(
				"diff_get_zlib_compression_level(): Encountered std::exception while trying to determine ZLIB: {}",
				msg);
		}
		else
		{
			ADU_LOG(
				"diff_get_zlib_compression_level(): Encountered unknown std::exception while trying to determine "
				"ZLIB.");
		}

		return static_cast<uint32_t>(errors::error_code::standard_library_exception);
	}
	catch (errors::user_exception &e)
	{
		ADU_LOG(
			"diff_get_zlib_compression_level(): Encountered user exception while trying to determine ZLIB "
			"compression: {}",
			e.get_message());
		return static_cast<uint32_t>(e.get_error());
	}
}

// This method attempts to validate that a blob was compressed with a given compression_level using ZLIB to perform GZ
// compression.
// Parameters:
//   uncompressed_reader: A reader to access the uncompressed data
//   compressed_reader: A reader to access the compressed data
//   compression_level: Compression level to validate against compressed data
// Return: Returns true if the data generated using the compression_level matches the data in the compressed_reader
bool try_recompress(io::reader &uncompressed_reader, io::reader &compressed_reader, int32_t compression_level)
{
	io::sequential::basic_reader_wrapper compressed_file_seq(compressed_reader);

	auto compressor_reader = io::compressed::zlib_compression_reader(
		uncompressed_reader,
		compression_level,
		uncompressed_reader.size(),
		compressed_reader.size(),
		io::compressed::zlib_helpers::init_type::gz);

	uint64_t bytes_verified                    = 0;
	uint64_t bytes_verified_last_milestone     = 0;
	uint64_t reported_bytes_verified_threshold = compressed_reader.size() / 10;

	uint64_t remaining = compressed_reader.size();
	std::vector<char> buffer_from_file;
	std::vector<char> buffer_from_compressor;

	const uint64_t chunk_size = 32 * 1024;
	buffer_from_file.resize(chunk_size);
	buffer_from_compressor.resize(chunk_size);

	// Pull data from the compressor until we have no more data to compare with the
	// compressed data.
	while (remaining)
	{
		// Read a blob from the compressed file and then read the same amount from the compressor
		// and then verify that they are identical.

		auto actual_read_from_file = compressed_file_seq.read_some(buffer_from_file);
		size_t actual_read_from_compressor;

		try
		{
			actual_read_from_compressor = compressor_reader.read_some(buffer_from_compressor);
		}
		catch (const errors::user_exception &e)
		{
			ADU_LOG(
				"try_recompress(): Failed to read from compressor for compression_level: {}. Exception: {}: {}",
				compression_level,
				static_cast<uint32_t>(e.get_error()),
				e.get_message());
			return false;
		}

		// Exceptional case - we read more data from the compressor than the
		// compressed data contains. Typically, if we are compressing more effectively
		// in the input data, we expect to see a mismatch in content before running
		// to the end of the entire compressed blob, but for very small files we
		// may hit this case.
		if (actual_read_from_compressor > remaining)
		{
			ADU_LOG(
				"try_recompress(): Read more data from compressor than expected for "
				"compression_level: {}",
				compression_level);
			return false;
		}

		// Exceptional case - we couldn't read enough data from the compressor to match the compressed file.
		// Like the above, this is more likely to occur with small files.
		if (actual_read_from_file != actual_read_from_compressor)
		{
			ADU_LOG(
				"try_recompress(): Read different sizes from file and compressor for "
				"compression_level: {}",
				compression_level);
			return false;
		}

		// The contents of the buffers don't match, so using this compression level won't replicate the
		// entire file identically. This is the standard failure case when we have the wrong compression_level.
		if (0 != memcmp(buffer_from_file.data(), buffer_from_compressor.data(), actual_read_from_file))
		{
			ADU_LOG(
				"try_recompress(): Read different data from file and compressor for "
				"compression_level: {}",
				compression_level);
			return false;
		}

		remaining -= actual_read_from_compressor;
		bytes_verified += actual_read_from_compressor;

		if ((bytes_verified - bytes_verified_last_milestone) >= reported_bytes_verified_threshold)
		{
			ADU_LOG("{}/{} bytes verified from compressor.", bytes_verified, compressed_reader.size());
			bytes_verified_last_milestone = bytes_verified;
		}
	}

	return true;
}