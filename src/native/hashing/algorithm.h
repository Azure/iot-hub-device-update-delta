/**
 * @file algorithm.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <string>
#include <vector>

#ifdef WIN32
	#include <memory>
#else
	#include <gcrypt.h>
#endif

namespace archive_diff::hashing
{
enum class algorithm : uint32_t
{
	invalid = 0,
	md5     = 32771,
	sha256  = 32780,
};

const algorithm all_algorithms[] = {algorithm::md5, algorithm::sha256};

#ifndef WIN32
int alg_to_gcrypt_algo(hashing::algorithm alg);
#endif

size_t get_byte_count_for_algorithm(algorithm algo);
} // namespace archive_diff::hashing
