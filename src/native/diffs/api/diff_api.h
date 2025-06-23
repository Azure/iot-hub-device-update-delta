#pragma once

#include "adudiffapi.h"
#include "aduapi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

ADUAPI_LINKAGESPEC uint32_t diff_get_zlib_compression_level(
	const char *uncompressed_file_path, const char *compressed_file_path, int32_t *compression_level);

#ifdef __cplusplus
}
#endif
