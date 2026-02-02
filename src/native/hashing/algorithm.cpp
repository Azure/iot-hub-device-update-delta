/**
 * @file algorithm.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "algorithm.h"

#include <errors/user_exception.h>

#ifndef USE_BCRYPT
#endif

namespace archive_diff::hashing
{
size_t get_byte_count_for_adu_algorithm(archive_diff::hashing::adu_algorithm algo)
{
	switch (algo)
	{
	case archive_diff::hashing::adu_algorithm::md5:
		return 16;
	case archive_diff::hashing::adu_algorithm::sha256:
		return 32;
	default:
		std::string msg = "diffs::hash::get_byte_count_for_adu_algorithm(): Unexpected hash type: "
		                + std::to_string(static_cast<int>(algo));
		throw errors::user_exception(errors::error_code::diff_bad_hash_type, msg);
	}
}

std::string get_adu_algorithm_name(archive_diff::hashing::adu_algorithm algo)
{
	switch (algo)
	{
	case archive_diff::hashing::adu_algorithm::md5:
		return "md5";
	case archive_diff::hashing::adu_algorithm::sha256:
		return "sha256";
	default:
		std::string msg =
			"diffs::hash::get_adu_algorithm_name(): Unexpected hash type: " + std::to_string(static_cast<int>(algo));
		throw errors::user_exception(errors::error_code::diff_bad_hash_type, msg);
	}
}

#ifdef USE_LIBGCRYPT
int adu_alg_to_gcrypt_algo(archive_diff::hashing::adu_algorithm alg)
{
	switch (alg)
	{
	case archive_diff::hashing::adu_algorithm::md5:
		return GCRY_MD_MD5;
	case archive_diff::hashing::adu_algorithm::sha256:
		return GCRY_MD_SHA256;
	default:
		std::string msg = "adu_alg_to_gcrypt_algo() has invalid value: " + std::to_string(static_cast<int>(alg));
		throw errors::user_exception(errors::error_code::hash_alg_to_gcrypt_algo, msg);
	}
}
#endif

} // namespace archive_diff::hashing