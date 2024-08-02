/**
 * @file hasher.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <string>
#include <vector>
#include <limits>

#include <errors/adu_log.h>
#include <errors/user_exception.h>

#include "hasher.h"
#include "hexstring_convert.h"

#ifdef WIN32

	#define WINDOWS_ENABLE_CPLUSPLUS 1
	#include <windows.h>
	#include <wincrypt.h>

	#undef max
#else

	#include <gpg-error.h>
#endif

namespace archive_diff::hashing
{
#ifdef WIN32

ALG_ID alg_to_BCRYPT_ALG_ID(algorithm alg)
{
	switch (alg)
	{
	case algorithm::md5:
		return CALG_MD5;
	case algorithm::sha256:
		return CALG_SHA_256;
	default:
		std::string msg = "alg_to_BCRYPT_ALG_ID() has invalid value: " + std::to_string(static_cast<int>(alg));
		throw errors::user_exception(errors::error_code::hash_alg_to_bcrypt_alg_id, msg);
	}
}

const wchar_t *ALG_ID_to_algorithm_name(ALG_ID alg_id)
{
	switch (alg_id)
	{
	case CALG_MD5:
		return BCRYPT_MD5_ALGORITHM;
	case CALG_SHA_256:
		return BCRYPT_SHA256_ALGORITHM;
	default:
		std::string msg = "ALG_ID_to_algorithm_name() has invalid value: " + std::to_string(static_cast<int>(alg_id));
		throw errors::user_exception(errors::error_code::hash_alg_id_to_algorithm_name, msg);
	}
}

void hasher::algorithm_provider_handle_deleter::operator()(void *ptr)
{
	BCryptCloseAlgorithmProvider(ptr, 0);
}

void hasher::hash_handle_deleter::operator()(void *ptr) { BCryptDestroyHash(ptr); }

#else
hasher::libgcrypt_initializer hashing::hasher::m_libgcrypt_initializer;

std::string get_libgcrypt_err_string(gcry_error_t err)
{
	const size_t INITIAL_ERR_STRING_SIZE = 50;
	const size_t MAX_ERR_STRING_SIZE     = 400;
	std::vector<char> err_string_vect{};
	err_string_vect.reserve(INITIAL_ERR_STRING_SIZE);

	while (true)
	{
		auto ret = gpg_strerror_r(err, err_string_vect.data(), err_string_vect.capacity());

		switch (ret)
		{
		case ERANGE:
			if (err_string_vect.capacity() < MAX_ERR_STRING_SIZE)
			{
				err_string_vect.reserve(err_string_vect.capacity() * 2);
				continue;
			}
		case 0:
			return std::string(err_string_vect.data());
		default:
			std::string msg = "get_libgcrypt_err_string: gpg_strerror_r() returned unexpected failure: ";
			msg += std::to_string(ret);
			throw errors::user_exception(errors::error_code::hash_failed_to_get_err_string, msg);
		}
	}
}

std::string get_libgcrypt_err_message(gcry_error_t err)
{
	std::string err_str{};

	err_str += gcry_strsource(err);
	err_str += "/";
	err_str += get_libgcrypt_err_string(err);
	err_str += "(";
	err_str += std::to_string(err);
	err_str += ")";

	return err_str;
}

std::string gcrypt_algo_to_string(int algo)
{
	switch (algo)
	{
	case GCRY_MD_MD5:
		return std::string("GCRY_MD_MD5");
	case GCRY_MD_SHA256:
		return std::string("GCRY_MD_SHA256");
	}

	std::string algo_string = "Unknown gcrypt algorithm: " + std::to_string(algo);
	return algo_string;
}

#endif

void hasher::reset()
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
		throw errors::user_exception(errors::error_code::hash_initialization_failure, msg);
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

		auto open_err = gcry_md_open(&m_hd, algo, 0);
		if (open_err)
		{
			std::string msg = "gcry_md_open(" + gcrypt_algo_to_string(algo) + ") failed: ";
			msg += get_libgcrypt_err_message(open_err);
			throw errors::user_exception(errors::error_code::hash_initialization_failure, msg);
		}
	}
#endif
}

hasher::hasher(algorithm alg) : m_alg(alg)
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
		throw errors::user_exception(errors::error_code::hash_provider_initialization_failure, msg);
	}
	m_algorithm_provider_handle.reset(provider_handle);
#endif
	reset();
}

hasher::~hasher()
{
#ifndef WIN32
	gcry_md_close(m_hd);
#endif
}

#define MIN_GCRYPT_VERSION "1.10.1"

#ifndef WIN32
hasher::libgcrypt_initializer::libgcrypt_initializer()
{
	ADU_LOG("Initializing libgcrypt. Requiring version: %s", MIN_GCRYPT_VERSION);

	if (!gcry_check_version(MIN_GCRYPT_VERSION))
	{
		std::string msg = "libgcrypt_initializer: gcry_check_version failed. Passed version: ";
		msg += MIN_GCRYPT_VERSION;
		throw errors::user_exception(errors::error_code::hash_libgcrypt_initialization_failure, msg);
	}

	gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

	ADU_LOG("Finished initializing libgcrypt.");
}
#endif

int hasher::hash_data(const void *data, size_t bytes)
{
#ifdef WIN32
	if (bytes > std::numeric_limits<ULONG>::max())
	{
		std::string msg = "Trying to hash more data than supported.";
		throw errors::user_exception(errors::error_code::hash_data_failure, msg);
	}

	auto bytesParam = static_cast<ULONG>(bytes);

	NTSTATUS ntstatus =
		BCryptHashData(m_hash_handle.get(), reinterpret_cast<PUCHAR>(const_cast<void *>(data)), bytesParam, 0);
	if (ntstatus)
	{
		std::string msg = "BCryptHashData() returned: " + std::to_string(ntstatus);
		throw errors::user_exception(errors::error_code::hash_data_failure, msg);
	}

#else
	gcry_md_write(m_hd, data, bytes);
#endif
	return 0;
}

std::vector<char> hasher::get_hash_binary()
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
		throw errors::user_exception(errors::error_code::hash_get_value, msg);
	}

	std::vector<unsigned char> hash_result;
	hash_result.reserve(hash_byte_count);
	ntstatus =
		BCryptFinishHash(m_hash_handle.get(), const_cast<unsigned char *>(hash_result.data()), hash_byte_count, 0);
	if (ntstatus)
	{
		std::string msg = "BCryptFinishHash() returned: " + std::to_string(ntstatus);
		throw errors::user_exception(errors::error_code::hash_get_value, msg);
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

std::string hasher::get_hash_string()
{
	auto hash_binary = get_hash_binary();
	return data_to_hexstring(hash_binary.data(), hash_binary.size());
}
}