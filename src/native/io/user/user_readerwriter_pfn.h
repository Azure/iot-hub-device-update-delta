/**
 * @file user_readerwriter_pfn.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

typedef void *(*user_readerwriter_create_pfn)(void *, const char *);

typedef size_t (*user_readerwriter_read_some_pfn)(void *, uint64_t, char *, size_t);
typedef uint8_t (*user_readerwriter_get_read_style_pfn)(void *);
typedef uint64_t (*user_readerwriter_size_pfn)(void *);
typedef int32_t (*user_readerwriter_write_pfn)(void *, uint64_t, const char *, uint64_t);
typedef int32_t (*user_readerwriter_flush_pfn)(void *);
typedef void (*user_readerwriter_close_pfn)(void *);

#ifdef __cplusplus
}
#endif