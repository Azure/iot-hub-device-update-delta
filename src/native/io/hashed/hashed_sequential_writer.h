/**
 * @file hashed_sequential_writer.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <functional>
#include <execution>

#include <hashing/hasher.h>
#include <io/sequential/writer_impl.h>
#include <io/sequential/basic_writer_wrapper.h>

namespace archive_diff::io::hashed
{
template <typename WriterT>
class hashed_sequential_writer_template : public io::sequential::writer_impl
{
	public:
	using SharedPtrWriterT   = std::shared_ptr<WriterT>;
	using SequentialWrapperT = io::sequential::basic_writer_wrapper_template<WriterT>;

	using this_type = hashed_sequential_writer_template<WriterT>;

	hashed_sequential_writer_template(SharedPtrWriterT &writer, std::shared_ptr<hashing::hasher> &hasher) :
		m_sequential_writer(std::make_shared<SequentialWrapperT>(writer))
	{
		auto write_op = [this](std::string_view data, [[maybe_unused]] hashing::hasher *hasher)
		{ m_sequential_writer->write(data); };

		m_hashers         = std::make_shared<std::vector<std::shared_ptr<hashing::hasher>>>();
		m_data_operations = std::make_shared<std::vector<data_op_entry>>();

		push_hasher(hasher);
		m_data_operations->push_back(data_op_entry{write_op, nullptr});
	}

	hashed_sequential_writer_template(
		hashed_sequential_writer_template &parent_hashed_writer, std::shared_ptr<hashing::hasher> &hasher)
	{
		m_sequential_writer = parent_hashed_writer.m_sequential_writer;

		m_hashers         = parent_hashed_writer.m_hashers;
		m_data_operations = parent_hashed_writer.m_data_operations;

		push_hasher(hasher);
	}

	virtual ~hashed_sequential_writer_template() { pop_hasher(); }

	/* io::writer */
	virtual void flush() override { m_sequential_writer->flush(); }

	/* io::sequential::writer */
	virtual void write(io::reader &reader) { io::sequential::writer::write(reader); }
	virtual void write(io::sequential::reader &reader) { io::sequential::writer::write(reader); }
	virtual void write(std::string_view buffer) override { io::sequential::writer_impl::write(buffer); }
	virtual uint64_t tellp() override { return m_sequential_writer->tellp(); }

	virtual void write_impl(std::string_view buffer) override
	{
		std::for_each(
			std::execution::par,
			m_data_operations->begin(),
			m_data_operations->end(),
			[&](data_op_entry &oper) { oper.pfn(buffer, oper.hasher); });
	}

	private:
	void push_hasher(std::shared_ptr<hashing::hasher> &hasher)
	{
		m_hashers->push_back(hasher);
		auto hash_op = [](std::string_view data, hashing::hasher *hasher) { hasher->hash_data(data); };
		m_data_operations->push_back(data_op_entry{hash_op, hasher.get()});
	}

	void pop_hasher()
	{
		m_hashers->pop_back();
		m_data_operations->pop_back();
	}

	std::shared_ptr<SequentialWrapperT> m_sequential_writer;

	// we update every hasher for each write
	// it shouldn't matter which writer we write to, we always hash all the values.
	std::shared_ptr<std::vector<std::shared_ptr<hashing::hasher>>> m_hashers;

	using data_op = std::function<void(std::string_view, hashing::hasher *hasher)>;
	struct data_op_entry
	{
		data_op pfn;
		hashing::hasher *hasher;
	};

	std::shared_ptr<std::vector<data_op_entry>> m_data_operations;
};

using hashed_sequential_writer = hashed_sequential_writer_template<io::writer>;
} // namespace archive_diff::io::hashed