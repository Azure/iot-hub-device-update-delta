/**
 * @file hexstring_convert.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */


#include "hexstring_convert.h"

namespace archive_diff::hashing
{
std::string data_to_hexstring(const char *data, size_t byte_count)
{
	const char *hex_digits = "0123456789abcdef";
	std::string hex_string;

	for (size_t i = 0; i < byte_count; i++)
	{
		unsigned char b = data[i];
		hex_string += hex_digits[b / 16];
		hex_string += hex_digits[b % 16];
	}
	return hex_string;
}

std::string hash_to_hexstring(const char *hash, [[maybe_unused]]hashing::algorithm alg)
{
#ifdef WIN32
	size_t hash_bytes = 0;
#else
	auto algo         = alg_to_gcrypt_algo(alg);
	size_t hash_bytes = gcry_md_get_algo_dlen(algo);
#endif
	return data_to_hexstring(hash, hash_bytes);
}
} // namespace archive_diff::hashing