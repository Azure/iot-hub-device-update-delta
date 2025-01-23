/**
 * @file algorithm.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <string>
#include <vector>

#ifdef USE_BCRYPT
	#include <memory>
#endif

#ifdef USE_LIBGCRYPT
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

#ifndef USE_BCRYPT
int alg_to_gcrypt_algo(hashing::algorithm alg);
#endif

std::string get_algorithm_name(algorithm algo);

size_t get_byte_count_for_algorithm(algorithm algo);
} // namespace archive_diff::hashing
