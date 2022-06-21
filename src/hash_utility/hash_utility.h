/**
 * @file hash_utility.h
 * @brief Implements utilities for working with hashes
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <string>
#include "gsl/span"
#include <vector>

#ifdef WIN32
	#include <memory>
#else
	#include <gcrypt.h>
#endif

std::string data_to_hexstring(const char *data, size_t byte_count);

namespace hash_utility
{
enum class algorithm
{
	MD5,
	SHA256,
};

class hasher
{
	public:
	hasher(algorithm alg);
	~hasher();
	void reset();
	int hash_data(gsl::span<char> data) { return hash_data(data.data(), data.size()); }
	int hash_data(std::string_view data) { return hash_data(data.data(), data.size()); }
	int hash_data(const void *data, size_t bytes);
	std::string get_hash_string();
	std::vector<char> get_hash_binary();

	private:
	algorithm m_alg;
#ifdef WIN32
	struct algorithm_provider_handle_deleter
	{
		void operator()(void *ptr);
	};

	struct hash_handle_deleter
	{
		void operator()(void *ptr);
	};

	using unique_algorithm_provider_handle = std::unique_ptr<void, algorithm_provider_handle_deleter>;
	using unique_hash_handle               = std::unique_ptr<void, hash_handle_deleter>;

	unique_algorithm_provider_handle m_algorithm_provider_handle;
	unique_hash_handle m_hash_handle;
#else
	gcry_md_hd_t m_hd{0};
#endif
};
} // namespace hash_utility
