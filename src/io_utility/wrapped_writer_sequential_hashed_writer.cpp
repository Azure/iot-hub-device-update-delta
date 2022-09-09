/**
 * @file wrapped_writer_sequential_hashed_writer.cpp
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <variant>
#include <functional>
#include <iostream>

#ifdef WIN32
	#define EXECUTION_SUPPORTED
#endif

#ifdef EXECUTION_SUPPORTED
	#include <execution>
#elif defined(_GLIBCXX_PARALLEL)
	#include <parallel/algorithm>
#endif

#include "wrapped_writer_sequential_hashed_writer.h"

using data_operation_variant = std::variant<hash_utility::hasher *, std::streambuf *>;

struct data_operation
{
	public:
	std::function<void(std::string_view data, data_operation_variant &value)> pfn;
	data_operation_variant value;
};

void hash_data_operation(std::string_view data, data_operation_variant &value)
{
	std::get<hash_utility::hasher *>(value)->hash_data(data);
}

void write_data_operation(std::string_view data, data_operation_variant &value)
{
	std::get<std::streambuf *>(value)->sputn(data.data(), data.size());
}

void io_utility::wrapped_writer_sequential_hashed_writer::write_and_hash_impl(
	std::string_view buffer, std::vector<hash_utility::hasher *> &hashers)
{
#ifdef DISABLE_ALL_HASHING
	streambuf->sputn(buffer.data(), buffer.size());
#else

	using data_op = std::function<void(std::string_view, hash_utility::hasher * hasher)>;
	struct data_op_entry
	{
		data_op pfn;
		hash_utility::hasher *hasher;
	};
	std::vector<data_op_entry> operations;

	auto write_op = [this](std::string_view data, hash_utility::hasher *hasher) { m_writer->write(data); };

	auto hash_op = [](std::string_view data, hash_utility::hasher *hasher) { hasher->hash_data(data); };

	operations.push_back(data_op_entry{write_op, nullptr});
	if (m_hasher != nullptr)
	{
		operations.push_back(data_op_entry{hash_op, m_hasher});
	}
	for (auto *other_hasher : hashers)
	{
		operations.push_back(data_op_entry{hash_op, other_hasher});
	}

	#ifdef EXECUTION_SUPPORTED
	std::for_each(
		std::execution::par,
		operations.begin(),
		operations.end(),
		[&](data_op_entry &oper) { oper.pfn(buffer, oper.hasher); });
	#elif defined(_GLIBCXX_PARALLEL)
	__gnu_parallel::for_each(
		operations.begin(), operations.end(), [&](data_op_entry &oper) { oper.pfn(buffer, oper.hasher); });
	#else
	std::for_each(operations.begin(), operations.end(), [&](data_op_entry &oper) { oper.pfn(buffer, oper.hasher); });
	#endif
#endif
}
