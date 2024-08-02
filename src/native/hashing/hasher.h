/**
 * @file hasher.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#pragma once

#include <string>
#include <vector>

#include <span>

#ifdef WIN32
	#include <memory>
#else
	#include <gcrypt.h>
#endif

#include "algorithm.h"
#include "hash.h"

namespace archive_diff::hashing
{
class hasher
{
	public:
	hasher(algorithm alg);
	~hasher();
	void reset();
	int hash_data(std::span<char> data) { return hash_data(data.data(), data.size()); }
	int hash_data(std::string_view data) { return hash_data(data.data(), data.size()); }

	int hash_data(const void *data, size_t bytes);
	std::string get_hash_string();
	std::vector<char> get_hash_binary();
	hash get_hash()
	{
		auto data = get_hash_binary();

		return hash::import_hash_value(m_alg, std::string_view{data.data(), data.size()});
	}

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

	class libgcrypt_initializer
	{
		public:
		libgcrypt_initializer();
	};

	static libgcrypt_initializer m_libgcrypt_initializer;

	gcry_md_hd_t m_hd{0};

#endif
};
} // namespace archive_diff::hashing
