/**
 * @file algorithm.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <cstdint>
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
enum  adu_algorithm
{
	invalid = 0,
	md5     = 32771,
	sha256  = 32780,
};


const archive_diff::hashing::adu_algorithm all_algorithms[] = {archive_diff::hashing::adu_algorithm::md5, archive_diff::hashing::adu_algorithm::sha256};

#ifndef USE_BCRYPT
int adu_alg_to_gcrypt_algo(archive_diff::hashing::adu_algorithm alg);
#endif

std::string get_adu_algorithm_name(archive_diff::hashing::adu_algorithm algo);

size_t get_byte_count_for_adu_algorithm(archive_diff::hashing::adu_algorithm algo);
} // namespace archive_diff::hashing
