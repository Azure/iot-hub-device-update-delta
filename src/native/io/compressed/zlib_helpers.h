/**
 * @file zlib_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <zlib.h>

#include <string>

#include <errors/user_exception.h>

namespace archive_diff::io::compressed
{
class zlib_helpers
{
	public:
	enum class init_type : uint64_t
	{
		raw  = 0,
		gz   = 1,
		zlib = 2,
	};

	static int get_init_bits(init_type init_type)
	{
		switch (init_type)
		{
		case init_type::raw:
			return -MAX_WBITS;
		case init_type::gz:
			return 16 + MAX_WBITS;
		case init_type::zlib:
			return MAX_WBITS;
		}

		std::string msg = "get_init_bits(): Invalid init_type: " + std::to_string(static_cast<int>(init_type));
		throw errors::user_exception(errors::error_code::io_zlib_init_type_invalid, msg);
	}
};

struct auto_zlib_cstream : public z_stream
{
	~auto_zlib_cstream() { ::deflateEnd(this); }
};

struct auto_zlib_dstream : public z_stream
{
	~auto_zlib_dstream() { ::inflateEnd(this); }
};
} // namespace archive_diff::io::compressed
