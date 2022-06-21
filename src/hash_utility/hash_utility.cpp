/**
 * @file hash_utility.cpp
 * @brief Implements utilities for working with hashes
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <string>
#include <vector>

#include "user_exception.h"

#include "hash_utility.h"

#ifdef WIN32

	#define WINDOWS_ENABLE_CPLUSPLUS 1
	#include <windows.h>
	#include <wincrypt.h>

	#undef max

#endif

#ifdef WIN32

ALG_ID alg_to_BCRYPT_ALG_ID(hash_utility::algorithm alg)
{
	switch (alg)
	{
	case hash_utility::algorithm::MD5:
		return CALG_MD5;
	case hash_utility::algorithm::SHA256:
		return CALG_SHA_256;
	}

	std::string msg = "alg_to_BCRYPT_ALG_ID() has invalid value: " + std::to_string(static_cast<int>(alg));
	throw error_utility::user_exception(error_utility::error_code::hash_alg_to_bcrypt_alg_id, msg);
}

const wchar_t *ALG_ID_to_algorithm_name(ALG_ID alg_id)
{
	switch (alg_id)
	{
	case CALG_MD5:
		return BCRYPT_MD5_ALGORITHM;
	case CALG_SHA_256:
		return BCRYPT_SHA256_ALGORITHM;
	}
	std::string msg = "ALG_ID_to_algorithm_name() has invalid value: " + std::to_string(static_cast<int>(alg_id));
	throw error_utility::user_exception(error_utility::error_code::hash_alg_id_to_algorithm_name, msg);
}

void hash_utility::hasher::algorithm_provider_handle_deleter::operator()(void *ptr)
{
	BCryptCloseAlgorithmProvider(ptr, 0);
}

void hash_utility::hasher::hash_handle_deleter::operator()(void *ptr) { BCryptDestroyHash(ptr); }

#else

int alg_to_gcrypt_algo(hash_utility::algorithm alg)
{
	switch (alg)
	{
	case hash_utility::algorithm::MD5:
		return GCRY_MD_MD5;
	case hash_utility::algorithm::SHA256:
		return GCRY_MD_SHA256;
	}

	std::string msg = "alg_to_gcrypt_algo() has invalid value: " + std::to_string(static_cast<int>(alg));
	throw error_utility::user_exception(error_utility::error_code::hash_alg_to_gcrypt_algo, msg);
}
#endif

std::string data_to_hexstring(const char *data, size_t byte_count)
{
	const char *hex_digits = "0123456789ABCDEF";
	std::string hex_string;

	for (size_t i = 0; i < byte_count; i++)
	{
		unsigned char b = data[i];
		hex_string += hex_digits[b / 16];
		hex_string += hex_digits[b % 16];
	}
	return hex_string;
}

std::string hash_to_hexstring(const char *hash, hash_utility::algorithm alg)
{
#ifdef WIN32
	size_t hash_bytes = 0;
#else
	auto algo         = alg_to_gcrypt_algo(alg);
	size_t hash_bytes = gcry_md_get_algo_dlen(algo);
#endif
	return data_to_hexstring(hash, hash_bytes);
}

void hash_utility::hasher::reset()
{
#ifdef WIN32
	#ifndef STATUS_SUCCESS
		#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
	#endif

	BCRYPT_HASH_HANDLE hash_handle;
	NTSTATUS ntstatus = BCryptCreateHash(
		m_algorithm_provider_handle.get(), &hash_handle, nullptr, 0, nullptr, 0, BCRYPT_HASH_REUSABLE_FLAG);
	if (ntstatus != STATUS_SUCCESS)
	{
		std::string msg = "BCryptCreateHash() returned: " + std::to_string(ntstatus);
		throw error_utility::user_exception(error_utility::error_code::hash_initialization_failure, msg);
	}
	m_hash_handle.reset(hash_handle);

#else
	if (m_hd)
	{
		gcry_md_reset(m_hd);
	}
	else
	{
		auto algo = alg_to_gcrypt_algo(m_alg);
		auto err  = gcry_md_open(&m_hd, algo, 0);

		if (err)
		{
			std::string msg = "gcry_md_open() returned: " + std::to_string(err);
			throw error_utility::user_exception(error_utility::error_code::hash_initialization_failure, msg);
		}
	}
#endif
}

hash_utility::hasher::hasher(hash_utility::algorithm alg) : m_alg(alg)
{
#ifdef WIN32
	#ifndef STATUS_SUCCESS
		#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
	#endif

	ALG_ID alg_id                 = alg_to_BCRYPT_ALG_ID(alg);
	const wchar_t *algorithm_name = ALG_ID_to_algorithm_name(alg_id);
	BCRYPT_ALG_HANDLE provider_handle;
	NTSTATUS ntstatus =
		BCryptOpenAlgorithmProvider(&provider_handle, algorithm_name, nullptr, BCRYPT_HASH_REUSABLE_FLAG);
	if (ntstatus != STATUS_SUCCESS)
	{
		std::string msg = "BCryptOpenAlgorithmProvider() returned: " + std::to_string(ntstatus);
		throw error_utility::user_exception(error_utility::error_code::hash_provider_initialization_failure, msg);
	}
	m_algorithm_provider_handle.reset(provider_handle);
#endif
	reset();
}

hash_utility::hasher::~hasher()
{
#ifndef WIN32
	gcry_md_close(m_hd);
#endif
}

int hash_utility::hasher::hash_data(const void *data, size_t bytes)
{
#ifdef WIN32
	if (bytes > std::numeric_limits<ULONG>::max())
	{
		std::string msg = "Trying to hash more data than supported.";
		throw error_utility::user_exception(error_utility::error_code::hash_data_failure, msg);
	}

	auto bytesParam = static_cast<ULONG>(bytes);

	NTSTATUS ntstatus =
		BCryptHashData(m_hash_handle.get(), reinterpret_cast<PUCHAR>(const_cast<void *>(data)), bytesParam, 0);
	if (ntstatus)
	{
		std::string msg = "BCryptHashData() returned: " + std::to_string(ntstatus);
		throw error_utility::user_exception(error_utility::error_code::hash_data_failure, msg);
	}

#else
	gcry_md_write(m_hd, data, bytes);
#endif
	return 0;
}

std::vector<char> hash_utility::hasher::get_hash_binary()
{
#ifdef WIN32
	DWORD hash_byte_count;
	DWORD result_byte_count;
	NTSTATUS ntstatus = BCryptGetProperty(
		m_hash_handle.get(),
		BCRYPT_HASH_LENGTH,
		reinterpret_cast<unsigned char *>(&hash_byte_count),
		sizeof(hash_byte_count),
		&result_byte_count,
		0);
	if (ntstatus)
	{
		std::string msg = "BCryptGetProperty() returned: " + std::to_string(ntstatus);
		throw error_utility::user_exception(error_utility::error_code::hash_get_value, msg);
	}

	std::vector<unsigned char> hash_result;
	hash_result.reserve(hash_byte_count);
	ntstatus =
		BCryptFinishHash(m_hash_handle.get(), const_cast<unsigned char *>(hash_result.data()), hash_byte_count, 0);
	if (ntstatus)
	{
		std::string msg = "BCryptFinishHash() returned: " + std::to_string(ntstatus);
		throw error_utility::user_exception(error_utility::error_code::hash_get_value, msg);
	}

	auto start = reinterpret_cast<char *>(hash_result.data());
	auto end   = &start[hash_byte_count];
	return std::vector<char>{start, end};
#else
	auto algo         = alg_to_gcrypt_algo(m_alg);
	auto start        = reinterpret_cast<char *>(gcry_md_read(m_hd, algo));
	size_t hash_bytes = gcry_md_get_algo_dlen(algo);
	auto end          = &start[hash_bytes];
	return std::vector<char>{start, end};
#endif
}

std::string hash_utility::hasher::get_hash_string()
{
	auto hash_binary = get_hash_binary();
	return data_to_hexstring(hash_binary.data(), hash_binary.size());
}
